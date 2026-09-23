import {readFile,writeFile} from 'node:fs/promises';
import path from 'node:path';
import assert from 'node:assert/strict';
assert(process.argv[2], 'Usage: node reaper_batch.mjs OUTPUT_DIRECTORY');
const directory=path.resolve(process.argv[2]);
for(const name of ['baseline','active','bypassed']) {
  const lines=(await readFile(path.join(directory,name+'.rpp'),'utf8')).split(/\r?\n/);
  function block(label) {
    const start=lines.findIndex(line=>line.trim()===`<${label}`);
    if(start<0)return '';
    let depth=0;
    for(let i=start;i<lines.length;i++) {
      if(lines[i].trim().startsWith('<'))depth++;
      if(lines[i].trim()==='>')depth--;
      if(!depth)return lines.slice(start,i+1).map(line=>line.trimStart()).join('\n');
    }
    throw new Error('Unclosed REAPER project block');
  }
  const format=block('RENDER_CFG').replace('<RENDER_CFG','<OUTFMT');
  assert(format, 'Missing render format in saved project');
  let fx=block('FXCHAIN');
  if(name!=='baseline'&&!fx.includes('VaginaPlugin'))throw new Error('Missing plugin in FX chain');
  if(name==='baseline') assert(!fx.includes('<VST'), 'Baseline must have no VST');
  if(name==='active') assert(/^BYPASS 0 0 0$/m.test(fx), 'Active plugin must be enabled and online');
  // Host bypass is serialized explicitly. A controller parameter changed while
  // the audio engine is idle may not yet be reflected in processor state.
  if(name==='bypassed') fx=fx.replace(/^BYPASS 0 0 0$/m,'BYPASS 1 0 0');
  if(name==='bypassed') assert(/^BYPASS 1 0 0$/m.test(fx), 'Host bypass must be enabled');
  const job=`${path.join(directory,'input.wav')}\t${path.join(directory,name+'.wav')}\n<CONFIG\nSRATE 48000\nNCH 2\nFX_NCH 2\nDITHER 0\n${format}\n${fx}\n>\n`;
  await writeFile(path.join(directory,name+'-batch.txt'),job);
}
