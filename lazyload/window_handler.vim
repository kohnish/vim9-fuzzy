vim9script

import "./job_handler.vim"

var CHANNEL_NULL: channel
var JOB_NULL: job
var g_channel = {"channel": CHANNEL_NULL, "job": JOB_NULL}
const g_script_dir = expand('<script>:p:h')
const g_preview_enabled = get(g:, 'vim9_fuzzy_enable_preview', false)
const g_original_preview_height = &previewheight
const g_max_rows = &lines
const g_one_third_row = float2nr(g_max_rows * 0.3)
const g_search_window_height = get(g:, 'vim9_fuzzy_win_height', g_one_third_row)
const g_file_preview_height = get(g:, 'vim9_fuzzy_file_preview_height', g_one_third_row * 2)
const g_yank_preview_height = get(g:, 'vim9_fuzzy_yank_preview_height', g_one_third_row)
const g_global_mru_enabled = get(g:, 'vim9_fuzzy_enable_global_mru', false)

const g_select_keymap = {
    "edit": get(g:, 'vim9_fuzzy_edit_key', "\<CR>"),
    "tabedit": get(g:, 'vim9_fuzzy_tabedit_key', "\<C-t>"),
    "botright_vsp": get(g:, 'vim9_fuzzy_botright_vsp_key', "\<C-]>"),
}

const g_yank_keymap = {
    "paste": get(g:, 'vim9_fuzzy_yank_paste_key', "\<CR>"),
    "copy": get(g:, 'vim9_fuzzy_yank_only_key', "\<C-t>"),
}

def Is_yank_and_select(input: string, ctx: dict<any>): bool
    if ctx.mode == "yank"
        if input == g_yank_keymap["paste"] || input == g_yank_keymap["copy"]
            return true
        endif
    endif
    return false
enddef

def DefaultGetGrepCmdStr(keyword: string, root_dir: string, target_dir: string): dict<any>
    var git_cmd = "git"
    if has("win64") || has("win32") || has("win16")
        git_cmd = git_cmd .. ".exe"
    endif
    if target_dir == ""
        return {
            "trim_target_dir": true,
            "cmd": "cd " .. root_dir .. " && git grep -n '" .. keyword .. "'",
        }
    endif
    return {
        "trim_target_dir": false,
        "cmd": "git grep -n '" .. keyword .. "'"
    }
enddef

def DefaultGetListCmdStr(root_dir: string, target_dir: string): dict<any>
    var rg_cmd = "rg"
    if has("win64") || has("win32") || has("win16")
        rg_cmd = rg_cmd .. ".exe"
    endif
    if target_dir == ""
        return {
            "trim_target_dir": true,
            "cmd": "cd " .. root_dir .. " && " .. rg_cmd .. " --files ",
        }
    endif
    return {
        "trim_target_dir": false,
        "cmd": rg_cmd .. " --files " .. target_dir,
    }
enddef

def DefaultRootdir(): string
    var root_dir = ""
    if exists('g:vim9_fuzzy_proj_dir') && g:vim9_fuzzy_proj_dir != "/" && getcwd() != "/"
        root_dir = g:vim9_fuzzy_proj_dir
    else
        root_dir = getcwd()
    endif
    return root_dir
enddef

const GetRootdir = get(g:, "Vim9_fuzzy_get_proj_root_func", () => DefaultRootdir())
const GetListCmdStr = get(g:, "Vim9_fuzzy_list_func", (root_dir_arg, target_dir_arg) => DefaultGetListCmdStr(root_dir_arg, target_dir_arg))
const GetGrepCmdStr = get(g:, "vim9_fuzzy_grep_func", (keyword, root_dir, target_dir) => DefaultGetGrepCmdStr(keyword, root_dir, target_dir))

def GetYankPath(): string
    var persist_path = ""
    if exists('g:vim9_fuzzy_yank_path')
        persist_path = g:vim9_fuzzy_yank_path
    else
        persist_path = g_script_dir .. "/../yank"
    endif
    return persist_path
enddef

def CreateCtx(persist_dir: string, root_dir: string, target_dir: string, mode: string, channel: dict<any>, buf_nr: number, orig_buf_id: number, orig_win_id: number): dict<any>
    var mru_path = ""
    if exists('g:vim9_fuzzy_mru_path')
        mru_path = g:vim9_fuzzy_mru_path
    else
        mru_path = persist_dir .. "/../mru"
    endif
    var yank_path = GetYankPath()

    # All const members
    return {
        "list_cmd": GetListCmdStr(root_dir, target_dir),
        "no_go_back": false,
        "orig_buf_id": orig_buf_id,
        "orig_win_id": orig_win_id,
        "buf_id": buf_nr,
        "root_dir": root_dir,
        "target_dir": target_dir,
        "mru_path": mru_path,
        "yank_path": yank_path,
        "mode": mode,
        "channel": channel.channel,
        "job": channel.job,
        "pedit_win": -1,
        "current_line": "",
    }
enddef

def IntToBin(n: number, fill_len: number): list<any>
    var bin_list = []
    var current_num = n
    var counter = 0
    while true
        if (current_num % 2) == 1
            add(bin_list, 1)
        else
            add(bin_list, 0)
        endif
        if current_num <= 1
            break
        endif
        current_num = current_num / 2
        counter += 1
    endwhile
    var rest = fill_len - counter
    if rest > 0
        for i in range(rest)
            add(bin_list, 0)
        endfor
    endif
    return reverse(bin_list)
enddef

var g_preview_timers = {}
def OpenPreviewForCurrentLine(ctx: dict<any>): void
    if !g_preview_enabled
        return
    endif
    # var timer = get(g_preview_timers, "preview", -1)
    # if !empty(timer_info(timer))
    #     timer_stop(timer)
    # endif
    # g_preview_timers["preview"] = timer_start(30, (_) => OpenPreviewForCurrentLineTask(ctx))
    OpenPreviewForCurrentLineTask(ctx)
enddef

# ToDo: Complete mess, clean up later.
def OpenPreviewForCurrentLineTask(ctx: dict<any>): void
    var line = getline(".")
    if ctx.mode == "yank"
        execute "setlocal previewheight=" .. g_yank_preview_height
        if !empty(line)
            var result_lines = split(line, "|")
            line = ctx.yank_path .. "/" .. result_lines[0]
        endif
    else
        execute "setlocal previewheight=" .. g_file_preview_height
    endif
    var grep_line_num = 0
    if ctx.mode == "grep"
        if !empty(line)
            var lines = split(line, ":")
            if len(lines) > 1
                if ctx.list_cmd["trim_target_dir"]
                    line = ctx.root_dir .. "/" .. lines[0]
                endif
                grep_line_num = str2nr(lines[1])
            endif
        endif
    elseif ctx.mode != "mru" && ctx.mode != "yank"
        if ctx.list_cmd["trim_target_dir"]
            line = ctx.root_dir .. "/" .. line
        endif
    endif
    var orig_bufs = tabpagebuflist(tabpagenr())
    var pedit_exec = "noautocmd silent topleft pedit "
    # pedit goes wrong on modified buffer
    try
        if filereadable(line)
            execute pedit_exec .. fnameescape(line)
        else
            execute pedit_exec  .. "VIM9_FUZZY_NULL"
        endif
    catch
    endtry
    if ctx.mode == "grep"
        var new_bufs = tabpagebuflist(tabpagenr())
        for i in new_bufs
            if index(orig_bufs, i) == -1
                ctx.pedit_win = bufwinid(i)
                break
            endif
        endfor
        try
            clearmatches(ctx.pedit_win)
        catch
        endtry
        var pedit_cmd = "call matchadd('Search', '" .. ctx.current_line .. "')"
        win_execute(ctx.pedit_win, pedit_cmd)
        win_execute(ctx.pedit_win, "call cursor(" .. grep_line_num .. ", 0)")
    endif
    execute "setlocal previewheight=" .. g_original_preview_height
enddef

def CountCharUntil(line: string, char: string): number
    var counter = 0
    for i in range(len(line))
        if line[i] != char
            counter += 1
        else
            break
        endif
    endfor
    return counter
enddef

def PrintResult(ctx: dict<any>, json_msg: dict<any>): void
    var buf_id = ctx.buf_id
    var win_id = bufwinid(ctx.buf_id)
    if win_id != -1
        clearmatches(bufwinid(ctx.buf_id))
    endif
    deletebufline(buf_id, 1, "$")
    if len(json_msg["result"]) != 0
        highlight matched_str_colour guifg=red ctermfg=red term=bold gui=bold
        var line_counter = 1
        var lines = []
        for i in json_msg["result"]
            add(lines, i["name"])
            var bin_list = IntToBin(i["match_pos"], len(i["name"]))
            var col_counter = 0
            for j in bin_list
                if ctx.mode == "yank" && col_counter < CountCharUntil(i["name"], '|') + 2
                    col_counter += 1
                    continue
                endif
                if j == 1
                    matchaddpos("matched_str_colour", [[line_counter, col_counter]], buf_id) 
                endif
                col_counter += 1
            endfor
            line_counter += 1
        endfor
        # ToDo: stop matching with the filename part
        if ctx.mode == "grep"
            matchadd("matched_str_colour", ctx.current_line)
        endif
        setbufline(buf_id, 1, lines)
    endif
    redraw
    OpenPreviewForCurrentLine(ctx)
    redraw
enddef

export def Write_mru(ctx: dict<any>, file_full_path: string): void
    var mru_msg = {"cmd": "write_mru", "mru_path": ctx.mru_path, "value": file_full_path }
    job_handler.WriteToChannel(ctx.channel, mru_msg, ctx, PrintResult)
enddef

def InitPrompt(): void
    echon "\r\r"
    echon ''
    echohl Constant | echon '>> ' | echohl NONE
enddef

def ConfigureWindow(ctx: dict<any>): void
    execute "resize " .. g_search_window_height

    setlocal statusline=\ \ Vim9\ Fuzzy
    setlocal nobuflisted
    setlocal buftype=nofile
    setlocal bufhidden=hide
    setlocal undolevels=-1
    setlocal noswapfile
    setlocal nolist
    setlocal norelativenumber
    setlocal nospell
    setlocal wrap
    setlocal nofoldenable
    setlocal foldmethod=manual
    setlocal shiftwidth=4
    setlocal cursorline
    setlocal cursorlineopt=both
    setlocal colorcolumn=
    setlocal number
    setlocal foldcolumn=0
    setlocal nowinfixheight
    setlocal filetype=

    InitPrompt()
    var cmd = "init_" .. ctx.mode
    var msg2send = {"cmd": cmd, "root_dir": ctx.root_dir, "list_cmd": ctx.list_cmd["cmd"], "mru_path": ctx.mru_path, "yank_path": ctx.yank_path}
    job_handler.WriteToChannel(ctx.channel, msg2send, ctx, PrintResult)
    redraw
enddef

def CloseWindow(ctx: dict<any>): void
    try
        clearmatches(ctx.buf_id)
    catch
    endtry
    try
        execute "silent bdelete! " .. ctx.buf_id
    catch
    endtry
    try
        clearmatches(ctx.pedit_win)
    catch
    endtry
    pclose
    echohl Normal | echon '' | echohl NONE
    redraw
    if !ctx.no_go_back
        win_gotoid(ctx.orig_win_id)
    endif
    if g_preview_enabled
        if !getbufvar(ctx.orig_buf_id, "&modified")
            try
                execute "e!"
            catch
            endtry
        endif
    endif
enddef

def SendCharMsg(ctx: dict<any>, msg: string): void
    ctx.current_line = msg
    var cmd = ctx.mode
    if cmd == "grep"
        ctx.list_cmd = GetGrepCmdStr(msg, ctx.root_dir, ctx.target_dir)
        var msg2send = {"cmd": cmd, "root_dir": ctx.root_dir, "list_cmd": ctx.list_cmd["cmd"], "value": msg, "mru_path": ctx.mru_path, "yank_path": ctx.yank_path}
        job_handler.WriteToChannel(ctx.channel, msg2send, ctx, PrintResult)
        return
    endif
    if len(msg) == 0
        cmd = "init_" .. cmd
    endif
    var msg2send = {"cmd": cmd, "root_dir": ctx.root_dir, "list_cmd": ctx.list_cmd["cmd"], "value": msg, "mru_path": ctx.mru_path, "yank_path": ctx.yank_path}
    job_handler.WriteToChannel(ctx.channel, msg2send, ctx, PrintResult)
enddef

def PrintFakePrompt(line: string, cursor_pos: number): void
    InitPrompt()
    if cursor_pos > 0
        echohl Normal | echon line[0 : cursor_pos - 1] | echohl NONE
    endif
    if cursor_pos != len(line)
        echohl Cursor | echon line[cursor_pos] | echohl NONE
    endif
    echohl Normal | echon line[cursor_pos + 1 : -1] | echohl NONE
    if cursor_pos == len(line)
        echohl Cursor | echon ' ' | echohl NONE
    else
        echohl Normal | echon ' ' | echohl NONE
    endif
    redraw
enddef

const WIN_ALREADY_FOCUSED = 0
const WIN_FOCUSED_ON_MODIFIABLE = 1
const WIN_NOT_FOUND = 2
def FocusIfOpen(filename: string): number
    var f_ret = WIN_NOT_FOUND
    for buf in getbufinfo()
        if buf.loaded && buf.name == filename && len(buf.windows) > 0
            win_gotoid(buf.windows[0])
            return WIN_ALREADY_FOCUSED
        elseif len(buf.windows) > 0 && getbufvar(buf.bufnr, '&buftype') != "terminal"
            if f_ret != WIN_FOCUSED_ON_MODIFIABLE
                win_gotoid(buf.windows[0])
                if !&modified
                    f_ret = WIN_FOCUSED_ON_MODIFIABLE
                endif
            endif
        endif
    endfor
    return f_ret
enddef

def GetFullPathFromResult(ctx: dict<any>, line: string, current_line: string): string
    var file_full_path = ""
    var base_dir = ctx.root_dir
    if !empty(ctx.target_dir)
        base_dir = ctx.target_dir
    endif
    if ctx.list_cmd["trim_target_dir"]
        file_full_path = base_dir .. "/" .. line
    else
        file_full_path = line
    endif

    # Let prompt behave like :e
    # ToDo: Make this configurable as well.
    if !filereadable(file_full_path)
        if filereadable(current_line)
            file_full_path = current_line
        endif
    endif

    # MRU is always stored as absolute path
    if ctx.mode == "mru"
        file_full_path = line
    endif

    if ctx.mode == "grep"
        var lines = split(line, ":")
        if len(lines) > 0
            if ctx.list_cmd["trim_target_dir"]
                file_full_path = ctx.root_dir .. "/" .. lines[0]
            else
                file_full_path = lines[0]
            endif
        endif
    endif

    return file_full_path
enddef

def FocusOrOpen(filename: string): void
    var f_ret = FocusIfOpen(filename)
    if f_ret == WIN_ALREADY_FOCUSED
        return
    endif
    if f_ret == WIN_FOCUSED_ON_MODIFIABLE
        execute "edit " .. filename
    else
        execute 'botright vsp ' .. filename
    endif
enddef

var g_pedit_timers = {"line": "", "timer": -1}
def BlockInput(ctx: dict<any>): void
    var current_line = ""
    var fake_cursor_position = 0
    while true
        var input = getcharstr()
        var input_num = char2nr(input)

        if input_num < 127 && input_num > 31
            if fake_cursor_position < len(current_line) && len(current_line) > 0
                if fake_cursor_position == 0
                    current_line = input .. current_line[0 : fake_cursor_position] .. current_line[fake_cursor_position + 1 : -1]
                    fake_cursor_position = fake_cursor_position + 1
                    PrintFakePrompt(current_line, fake_cursor_position)
                else
                    current_line = current_line[0 : fake_cursor_position - 1] .. input .. current_line[fake_cursor_position : -1]
                    fake_cursor_position = fake_cursor_position + 1
                    PrintFakePrompt(current_line, fake_cursor_position)
                endif
            elseif fake_cursor_position > 0
                current_line = current_line[0 : fake_cursor_position] .. input .. current_line[fake_cursor_position + 1 : -1]
                fake_cursor_position = fake_cursor_position + 1
                PrintFakePrompt(current_line, fake_cursor_position)
            else
                if current_line == ""
                    current_line = input .. current_line
                    fake_cursor_position = fake_cursor_position + 1
                    PrintFakePrompt(current_line, fake_cursor_position)
                else
                    current_line = input .. current_line
                    PrintFakePrompt(current_line, fake_cursor_position)
                endif
            endif
            SendCharMsg(ctx, current_line)
        elseif input == "\<Left>" || input == "\<C-h>"
            if fake_cursor_position > 0
                fake_cursor_position = fake_cursor_position - 1
                PrintFakePrompt(current_line, fake_cursor_position)
            endif
        elseif input == "\<HOME>"
            fake_cursor_position = 0
            PrintFakePrompt(current_line, fake_cursor_position)
        elseif input == "\<END>"
            fake_cursor_position = len(current_line)
            PrintFakePrompt(current_line, fake_cursor_position)
        elseif input == "\<Right>" || input == "\<C-l>"
            if fake_cursor_position <= len(current_line) - 1
                InitPrompt()
                fake_cursor_position = fake_cursor_position + 1
                PrintFakePrompt(current_line, fake_cursor_position)
            endif
        elseif input == "\<BS>"
            if fake_cursor_position < 1
                continue
            elseif fake_cursor_position == len(current_line)
                current_line = current_line[0 : -2]
                SendCharMsg(ctx, current_line)
                fake_cursor_position = fake_cursor_position - 1
                PrintFakePrompt(current_line, fake_cursor_position)
            elseif fake_cursor_position > 1
                var current_line_first = current_line[0 : fake_cursor_position - 2]
                var current_line_last = current_line[fake_cursor_position  : -1]
                current_line = current_line_first .. current_line_last
                SendCharMsg(ctx, current_line)
                fake_cursor_position = fake_cursor_position - 1
                PrintFakePrompt(current_line, fake_cursor_position)
            elseif fake_cursor_position == 1
                current_line = current_line[1  : -1]
                SendCharMsg(ctx, current_line)
                fake_cursor_position = fake_cursor_position - 1
                PrintFakePrompt(current_line, fake_cursor_position)
            endif
            clearmatches(bufwinid(ctx.buf_id))
        elseif input == "\<DEL>"
            if fake_cursor_position == 0 
                current_line = current_line[fake_cursor_position + 1 : -1]
                SendCharMsg(ctx, current_line)
                PrintFakePrompt(current_line, fake_cursor_position)
            elseif fake_cursor_position < len(current_line)
                var current_line_first = current_line[0 : fake_cursor_position - 1]
                var current_line_last = current_line[fake_cursor_position + 1 : -1]
                current_line = current_line_first .. current_line_last
                SendCharMsg(ctx, current_line)
                PrintFakePrompt(current_line, fake_cursor_position)
            endif
            clearmatches(bufwinid(ctx.buf_id))
        elseif input == "\<C-j>" || input == "\<C-n>" || input == "\<ScrollWheelDown>" || input == "\<Down>"
            normal j
            OpenPreviewForCurrentLine(ctx)
            redraw
        elseif input == "\<Up>" || input == "\<ScrollWheelUp>" || input == "\<C-k>" || input == "\<C-p>"
            normal k
            OpenPreviewForCurrentLine(ctx)
            redraw
        elseif input == "\<ESC>"
            break
        elseif index(values(g_select_keymap), input) >= 0 || Is_yank_and_select(input, ctx) == true
            if current_line == ":q"
                break
            endif

            var line = getline(".")
            if empty(line)
                continue
            endif

            if ctx.mode == "yank"
                var for_paste = getline('.')
                var result_lines = split(for_paste, "|")
                var file_name = ctx.yank_path .. "/" .. result_lines[0]
                var lines_for_paste = readfile(file_name)
                if input == g_yank_keymap["paste"]
                    appendbufline(ctx.orig_buf_id, line('.'), lines_for_paste)
                elseif input == g_yank_keymap["copy"]
                    system("printf $'\\e]52;c;%s\\a' \"$(base64 <<(</dev/stdin))\" >> /dev/tty", lines_for_paste)
                else
                    continue
                endif
                break
            endif

            var file_full_path = fnamemodify(GetFullPathFromResult(ctx, line, current_line), ':p')

            if filereadable(file_full_path)
                if !g_global_mru_enabled
                    var mru_msg = {"cmd": "write_mru", "mru_path": ctx.mru_path, "value": file_full_path }
                    job_handler.WriteToChannel(ctx.channel, mru_msg, ctx, PrintResult)
                endif
                # Close here, focus goes all wrong.
                # CloseWindow gets called again, after the loop finishes...
                CloseWindow(ctx)
                if input == g_select_keymap["edit"]
                    FocusOrOpen(file_full_path)
                elseif input == g_select_keymap["botright_vsp"]
                    ctx.no_go_back = true
                    execute 'botright vsp ' .. file_full_path
                elseif input == g_select_keymap["tabedit"]
                    ctx.no_go_back = true
                    execute 'tabedit ' .. file_full_path
                endif
                if ctx.mode == "grep"
                    var lines = split(line, ":")
                    if len(lines) > 1
                        cursor(str2nr(lines[1]), 0)
                    endif
                endif
            else
                echom "File: " .. file_full_path .. " cannot be accessed"
            endif
            break
        else
            continue
        endif
    endwhile
enddef

def InitProcess(): dict<any>
    if ch_status(g_channel.channel) != "open"
        g_channel = job_handler.StartFinderProcess()
    endif
    return g_channel
enddef

export def Global_mru_write(): void
    var file_path = expand('%:p')
    if filereadable(file_path)
        var channel = InitProcess()
        var ctx = CreateCtx(g_script_dir, "", "", "", channel, -1, -1, -1)
        var mru_msg = {"cmd": "write_mru", "mru_path": ctx.mru_path, "value": file_path }
        job_handler.WriteToChannel(ctx.channel, mru_msg, ctx, PrintResult)
    endif
enddef

export def StartWindow(...args: list<string>): void
    var mode = args[0]
    var target_dir = ""
    if len(args) > 1
        target_dir = args[1]
    endif
    var channel = InitProcess()
    var orig_buf_id = bufnr()
    noswapfile noautocmd keepalt keepjumps botright split Vim9 Fuzzy
    var ctx = CreateCtx(g_script_dir, GetRootdir(), target_dir, mode, channel, bufnr(), orig_buf_id, bufwinid(orig_buf_id))
    ConfigureWindow(ctx)
    try
        BlockInput(ctx)
    finally
        CloseWindow(ctx)
    endtry
enddef

export def Osc52YankHist(contents: list<any>): void
    var yank_path = GetYankPath()
    mkdir(yank_path, "p")

    if len(contents[0]) > 1 || len(contents) > 1
        var yank_file =  yank_path .. "/" .. strftime("%d-%b-%Y %T ")
        writefile(contents, yank_file, 'b')
    endif
    var files = split(globpath(yank_path, '*'), '\n')
    # ToDo: make the max num configurable and stop using unix commands.
    if len(files) > 50
        system("rm -f \"`ls -1tr " .. yank_path .. "/* |head -n1`\"")
    endif
enddef

export def StopVim9Fuzzy(): void
    if ch_status(g_channel.channel) == "open"
        job_stop(g_channel.job, "int")
        echom "Stopped vim9-fuzzy"
    else
        echom "Vim9-fuzzy is already stopped"
    endif
enddef
