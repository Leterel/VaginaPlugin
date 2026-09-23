-- Run only in a disposable REAPER instance with -cfgfile pointing at output/reaper.ini.
-- Creates synthetic test projects. Never opens an existing user project or starts playback.
local output = assert(os.getenv('VAGINA_TEST_OUTPUT'), 'VAGINA_TEST_OUTPUT is required')
local function normalized(s) return s:gsub('\\', '/'):gsub('/$', ''):lower() end
assert(normalized(reaper.GetResourcePath()) == normalized(output), 'Refusing non-isolated REAPER profile')
assert(reaper.CountTracks(0) == 0, 'Refusing a nonempty project')
local log = assert(io.open(output .. '/host-results.txt', 'w'))
local function note(text) log:write(text, '\n'); log:flush() end
local function check(ok, message) if not ok then note('FAIL: ' .. message); error(message) end end
note('REAPER ' .. reaper.GetAppVersion())
note('Isolated profile and empty project verified')
reaper.InsertMedia(output .. '/input.wav', 0)
check(reaper.CountTracks(0) == 1, 'Synthetic audio track created')
local track = reaper.GetTrack(0, 0)
local item = reaper.GetTrackMediaItem(track, 0)
check(item ~= nil, 'Synthetic audio item exists')
reaper.SetMediaItemInfo_Value(item, 'D_FADEINLEN', 0)
reaper.SetMediaItemInfo_Value(item, 'D_FADEOUTLEN', 0)
reaper.SetMediaTrackInfo_Value(track, 'D_VOL', 1)
reaper.SetMediaTrackInfo_Value(reaper.GetMasterTrack(0), 'D_VOL', 1)
for key,value in pairs({RENDER_SETTINGS=0, RENDER_BOUNDSFLAG=0, RENDER_STARTPOS=0,
  RENDER_ENDPOS=3, RENDER_CHANNELS=2, RENDER_SRATE=48000, RENDER_TAILFLAG=0,
  RENDER_DITHER=0, RENDER_NORMALIZE=0, RENDER_ADDTOPROJ=0, PROJECT_SRATE=48000,
  PROJECT_SRATE_USE=1}) do reaper.GetSetProjectInfo(0, key, value, true) end
reaper.GetSetProjectInfo_String(0, 'RENDER_FILE', output, true)
reaper.GetSetProjectInfo_String(0, 'RENDER_FORMAT', 'evaw', true)
local function save(name)
  reaper.GetSetProjectInfo_String(0, 'RENDER_PATTERN', name, true)
  reaper.Main_SaveProjectEx(0, output .. '/' .. name .. '.rpp', 0)
  note('Saved ' .. name .. '.rpp')
end
save('baseline')
local fx = reaper.TrackFX_AddByName(track, 'VST3: VaginaPlugin (Leterel)', false, -1)
check(fx >= 0, 'VaginaPlugin instantiated')
local ok,name = reaper.TrackFX_GetFXName(track, fx)
check(ok and name:find('VaginaPlugin', 1, true), 'Correct plugin identified')
check(not reaper.TrackFX_GetOffline(track, fx), 'Plugin is online')
note('Loaded ' .. name)
local bypass = nil
for p=0,reaper.TrackFX_GetNumParams(track, fx)-1 do
  local _,label = reaper.TrackFX_GetParamName(track, fx, p)
  if label == 'Bypass visualizer' then bypass = p end
end
check(bypass ~= nil, 'Visualizer bypass parameter exposed')
save('active')
reaper.TrackFX_SetParamNormalized(track, fx, bypass, 1)
check(reaper.TrackFX_GetParamNormalized(track, fx, bypass) == 1, 'Bypass accepted')
reaper.TrackFX_SetEnabled(track, fx, false)
save('bypassed')
reaper.TrackFX_SetEnabled(track, fx, true)
reaper.TrackFX_SetParamNormalized(track, fx, bypass, 0)

if os.getenv('VAGINA_TEST_EDITOR') ~= '1' then
  save('active')
  note('PASS: real DAW load, bypass parameter and render projects (editor cycles not requested)')
  log:close()
  reaper.Main_OnCommand(40004, 0)
  return
end

local cycles,shown,nextTime = 0,false,reaper.time_precise()
local function editorCycle()
  if reaper.time_precise() < nextTime then reaper.defer(editorCycle); return end
  if not shown then
    reaper.TrackFX_Show(track, fx, 3)
    shown = true
  else
    if reaper.TrackFX_GetFloatingWindow(track, fx) == nil then
      note('INCOMPLETE: floating editor unavailable at cycle ' .. (cycles + 1))
      log:close()
      reaper.Main_SaveProjectEx(0, output .. '/editor-interrupted.rpp', 0)
      reaper.Main_OnCommand(40004, 0)
      return
    end
    reaper.TrackFX_Show(track, fx, 2)
    cycles = cycles + 1; shown = false
    note('Editor open/close cycle ' .. cycles .. ' passed')
  end
  if cycles < 10 then nextTime = reaper.time_precise() + 0.15; reaper.defer(editorCycle); return end
  save('active')
  note('PASS: real DAW load, bypass state, 10 editor cycles and render projects')
  log:close()
  reaper.Main_OnCommand(40004, 0)
end
reaper.defer(editorCycle)
