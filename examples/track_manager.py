sys.path.append(RPR_GetResourcePath() + "/Scripts/ReaTeam Extensions/API")
from imgui_python import *

FLT_MIN = 1.17549e-38
ctx = ImGui_CreateContext("Track manager", 700, 500)[0]

def paramCheckbox(track, param):
  value = RPR_GetMediaTrackInfo_Value(track, param)
  checkbox = ImGui_Checkbox(ctx, "##" + param, value)
  if checkbox[0]:
    RPR_SetMediaTrackInfo_Value(track, param, checkbox[3])
    return True

  return False

def trackRow(ti):
  track = RPR_GetTrack(None, ti)
  color = RPR_GetTrackColor(track)

  if ImGui_TableSetColumnIndex(ctx, 0):
    colorEdit = ImGui_ColorEdit3(ctx, "##color", color,
      ImGui_ColorEditFlags_NoInputs() | ImGui_ColorEditFlags_NoLabel())
    if colorEdit[0]:
      RPR_SetTrackColor(track, colorEdit[3])

  if ImGui_TableSetColumnIndex(ctx, 1):
    selected = RPR_IsTrackSelected(track)
    selectable = ImGui_Selectable(ctx, ti + 1, selected,
      ImGui_SelectableFlags_SpanAllColumns() |
      ImGui_SelectableFlags_AllowItemOverlap())
    if selectable[0]:
      if ImGui_GetKeyMods(ctx) & ImGui_KeyModFlags_Ctrl():
        RPR_SetTrackSelected(track, not selected)
      else:
        RPR_SetOnlyTrackSelected(track)

  if ImGui_TableSetColumnIndex(ctx, 2):
    name = RPR_GetSetMediaTrackInfo_String(track, 'P_NAME', '', False)[3]
    ImGui_SetNextItemWidth(ctx, -FLT_MIN)
    inputText = ImGui_InputText(ctx, "##name", name, 0)
    if inputText[0]:
      RPR_GetSetMediaTrackInfo_String(track, 'P_NAME', inputText[3], True)

  if ImGui_TableSetColumnIndex(ctx, 3):
    if paramCheckbox(track, 'B_SHOWINTCP'):
      RPR_TrackList_AdjustWindows(True)

  if ImGui_TableSetColumnIndex(ctx, 4):
    if paramCheckbox(track, 'B_SHOWINMIXER'):
      RPR_TrackList_AdjustWindows(False)

  if ImGui_TableSetColumnIndex(ctx, 5):
    fxCount = RPR_TrackFX_GetCount(track)
    if ImGui_Selectable(ctx, fxCount if fxCount > 0 else '')[0]:
      RPR_TrackFX_Show(track, 0, 1)

  if ImGui_TableSetColumnIndex(ctx, 6):
    fxCount = RPR_TrackFX_GetRecCount(track)
    if fxCount > 0:
      ImGui_Text(ctx, fxCount)

  if ImGui_TableSetColumnIndex(ctx, 7):
    paramCheckbox(track, 'I_RECARM')
  if ImGui_TableSetColumnIndex(ctx, 8):
   paramCheckbox(track, 'B_MUTE')
  if ImGui_TableSetColumnIndex(ctx, 9):
    paramCheckbox(track, 'I_SOLO')
  if ImGui_TableSetColumnIndex(ctx, 10):
    paramCheckbox(track, 'B_HEIGHTLOCK')
  if ImGui_TableSetColumnIndex(ctx, 11):
    if paramCheckbox(track, 'B_FREEMODE'):
      RPR_UpdateArrange()

def trackTable():
  flags = ImGui_TableFlags_SizingFixedFit() | ImGui_TableFlags_RowBg()   | \
          ImGui_TableFlags_BordersOuter()   | ImGui_TableFlags_Borders() | \
          ImGui_TableFlags_Reorderable()    | ImGui_TableFlags_ScrollY() | \
          ImGui_TableFlags_Resizable()      | ImGui_TableFlags_Hideable()

  if not ImGui_BeginTable(ctx, "tracks", 12, flags)[0]:
    return

  ImGui_TableSetupScrollFreeze(ctx, 0, 1)

  ImGui_TableSetupColumn(ctx, "Color",       ImGui_TableColumnFlags_NoHeaderWidth())
  ImGui_TableSetupColumn(ctx, "#",           ImGui_TableColumnFlags_None())
  ImGui_TableSetupColumn(ctx, "Name",        ImGui_TableColumnFlags_WidthStretch())
  ImGui_TableSetupColumn(ctx, "TCP",         ImGui_TableColumnFlags_None())
  ImGui_TableSetupColumn(ctx, "MCP",         ImGui_TableColumnFlags_None())
  ImGui_TableSetupColumn(ctx, "FX",          ImGui_TableColumnFlags_None())
  ImGui_TableSetupColumn(ctx, "IN-FX",       ImGui_TableColumnFlags_None())
  ImGui_TableSetupColumn(ctx, "R",           ImGui_TableColumnFlags_None())
  ImGui_TableSetupColumn(ctx, "M",           ImGui_TableColumnFlags_None())
  ImGui_TableSetupColumn(ctx, "S",           ImGui_TableColumnFlags_None())
  ImGui_TableSetupColumn(ctx, "Height Lock", ImGui_TableColumnFlags_NoHeaderWidth())
  ImGui_TableSetupColumn(ctx, "FIPM",        ImGui_TableColumnFlags_NoHeaderWidth())
  ImGui_TableHeadersRow(ctx)

  trackCount = RPR_CountTracks(None)
  clipper = ImGui_CreateListClipper(ctx)
  ImGui_ListClipper_Begin(clipper, trackCount)
  while ImGui_ListClipper_Step(clipper):
    _, displayStart, displayEnd = ImGui_ListClipper_GetDisplayRange(clipper)
    for ti in range(displayStart, displayEnd):
      ImGui_TableNextRow(ctx)
      ImGui_PushID(ctx, ti)
      trackRow(ti)
      ImGui_PopID(ctx)

  ImGui_EndTable(ctx)

def loop():
  if ImGui_IsCloseRequested(ctx):
    ImGui_DestroyContext(ctx)
    return

  ImGui_SetNextWindowPos(ctx, 0, 0)
  _, w, h = ImGui_GetDisplaySize(ctx)
  ImGui_SetNextWindowSize(ctx, w, h)
  ImGui_Begin(ctx, "main", None, ImGui_WindowFlags_NoDecoration())

  if ImGui_Button(ctx, "Add track")[0]:
    RPR_InsertTrackAtIndex(-1, True)

  trackTable()

  ImGui_End(ctx)

  RPR_defer("loop()")

RPR_defer("loop()")
