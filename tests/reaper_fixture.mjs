import { writeFile, readFile } from 'node:fs/promises';
import assert from 'node:assert/strict';
import path from 'node:path';
const directory = process.argv[3];
assert(directory, 'Usage: node reaper_fixture.mjs create|verify OUTPUT_DIRECTORY');
if (process.argv[2] === 'create') {
  const rate=48000, frames=rate*3, data=Buffer.alloc(frames*4);
  const tones=[60,250,1000,3500,10000];
  for(let frame=0;frame<frames;frame++) {
    const t=frame/rate;
    const active=t>=0.25&&t<2.75;
    for(let ch=0;ch<2;ch++) {
      const sum=active?tones.reduce((v,hz,i)=>v+Math.sin(2*Math.PI*hz*t+(ch?i*0.37:0)),0)*0.08:0;
      data.writeInt16LE(Math.round(sum*32767),frame*4+ch*2);
    }
  }
  const header=Buffer.alloc(44);
  header.write('RIFF');header.writeUInt32LE(data.length+36,4);header.write('WAVEfmt ',8);
  header.writeUInt32LE(16,16);header.writeUInt16LE(1,20);header.writeUInt16LE(2,22);
  header.writeUInt32LE(rate,24);header.writeUInt32LE(rate*4,28);header.writeUInt16LE(4,32);
  header.writeUInt16LE(16,34);header.write('data',36);header.writeUInt32LE(data.length,40);
  await writeFile(path.join(directory,'input.wav'),Buffer.concat([header,data]));
} else if (process.argv[2] === 'verify') {
  async function wave(name) {
    const bytes=await readFile(path.join(directory,name+'.wav'));
    assert.equal(bytes.toString('ascii',0,4),'RIFF');assert.equal(bytes.toString('ascii',8,12),'WAVE');
    let format,data;
    for(let off=12;off+8<=bytes.length;) {
      const size=bytes.readUInt32LE(off+4),id=bytes.toString('ascii',off,off+4);
      assert(off+8+size<=bytes.length,'Truncated WAV chunk');
      if(id==='fmt ') format=bytes.subarray(off+8,off+8+size);
      if(id==='data') data=bytes.subarray(off+8,off+8+size);
      off+=8+size+(size%2);
    }
    assert(format&&data&&data.length>0,'Missing WAV audio');return {format,data};
  }
  const baseline=await wave('baseline'),active=await wave('active'),bypassed=await wave('bypassed');
  assert(baseline.data.some(byte=>byte!==0),'Baseline must contain non-silent audio');
  assert(baseline.format.equals(active.format)&&baseline.format.equals(bypassed.format),'Matching render format');
  assert(baseline.data.equals(active.data),'Active VST3 changed rendered audio');
  assert(baseline.data.equals(bypassed.data),'Bypassed VST3 changed rendered audio');
  const result={testedAt:new Date().toISOString(),sampleRate:baseline.format.readUInt32LE(4),channels:baseline.format.readUInt16LE(2),bits:baseline.format.readUInt16LE(14),pcmBytes:baseline.data.length,activeBitIdentical:true,bypassedBitIdentical:true,limitations:['Offline rendering with synthetic audio','No physical soundcard or live playback test']};
  await writeFile(path.join(directory,'audio-results.json'),JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify(result));
} else throw new Error('Unknown command');
