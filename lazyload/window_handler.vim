vim9script

import "./job_handler.vim"

var CHANNEL_NULL: channel
var JOB_NULL: job
var g_channel = {"channel": CHANNEL_NULL, "job": JOB_NULL}
const g_script_dir = expand('<script>:p:h')


const g_select_keymap = {
    "edit": get(g:, 'vim9_fuzzy_edit_key', "\<CR>"),
    "tabedit": get(g:, 'vim9_fuzzy_tabedit_key', "\<C-t>"),
    "botright_vsp": get(g:, 'vim9_fuzzy_botright_vsp_key', "\<C-]>")
}

def GetListCmdStr(root_dir: string, target_dir: string): dict<any>
    if exists('g:vim9_fuzzy_list_func') && g:vim9_fuzzy_list_func
        return g:Vim9_fuzzy_list_func(root_dir, target_dir)
    endif

    # Default
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

def GetRootdir(): string
    var root_dir = ""
    if exists('g:vim9_fuzzy_get_proj_root_func') && g:vim9_fuzzy_get_proj_root_func
        root_dir = g:Vim9_fuzzy_get_proj_root_func()
    elseif exists('g:vim9_fuzzy_proj_dir') && g:vim9_fuzzy_proj_dir != "/" && getcwd() != "/"
        root_dir = g:vim9_fuzzy_proj_dir
    else
        root_dir = getcwd()
    endif
    return root_dir
enddef

def GetYankPath(): string
    var persist_path = ""
    if exists('g:vim9_fuzzy_yank_path')
        persist_path = g:vim9_fuzzy_yank_path
    else
        persist_path = g_script_dir .. "/../yank"
    endif
    return persist_path
enddef

def CreateCfg(persist_dir: string, root_dir: string, target_dir: string, mode: string, channel: dict<any>): dict<any>
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
        "root_dir": root_dir,
        "target_dir": target_dir,
        "mru_path": mru_path,
        "yank_path": yank_path,
        "mode": mode,
        "channel": channel.channel,
        "job": channel.job,
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

export def PrintResult(ctx: dict<any>, json_msg: dict<any>): void
    var buf_id = bufnr("Vim9 Fuzzy")
    if buf_id == -1
        return
    endif
    deletebufline(buf_id, 1, "$")
    if len(json_msg["result"]) != 0
        clearmatches()
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
        setbufline(buf_id, 1, lines)
    endif
    redraw
enddef

def InitPrompt(): void
    echon "\r\r"
    echon ''
    echohl Constant | echon '>> ' | echohl NONE
enddef

def InitWindow(cfg: dict<any>): void
    noswapfile noautocmd keepalt keepjumps botright split Vim9 Fuzzy
    execute "resize " .. get(g:, 'vim9_fuzzy_win_height', 20)

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
    var cmd = "init_" .. cfg.mode
    var msg2send = {"cmd": cmd, "root_dir": cfg.root_dir, "list_cmd": cfg.list_cmd["cmd"], "mru_path": cfg.mru_path, "yank_path": cfg.yank_path}
    job_handler.WriteToChannel(cfg.channel, msg2send, cfg, PrintResult)
    redraw
enddef

def CloseWindow(): void
    bdelete
    echohl Normal | echon '' | echohl NONE
    redraw
enddef

def SendCharMsg(cfg: dict<any>, msg: string): void
    var cmd = cfg.mode
    if len(msg) == 0
        cmd = "init_" .. cmd
    endif
    var msg2send = {"cmd": cmd, "root_dir": cfg.root_dir, "list_cmd": cfg.list_cmd["cmd"], "value": msg, "mru_path": cfg.mru_path, "yank_path": cfg.yank_path}
    job_handler.WriteToChannel(cfg.channel, msg2send, cfg, PrintResult)
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

def GetFullPathFromResult(cfg: dict<any>, line: string, current_line: string): string
    var file_full_path = ""
    var base_dir = cfg.root_dir
    if !empty(cfg.target_dir)
        base_dir = cfg.target_dir
    endif
    if cfg.list_cmd["trim_target_dir"]
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
    if cfg.mode == "mru"
        file_full_path = line
    endif

    return file_full_path
enddef

def FocusOrOpen(filename: string): void
    var f_ret = FocusIfOpen(filename)
    if f_ret == WIN_FOCUSED_ON_MODIFIABLE
        execute "edit " .. filename
    elseif f_ret == WIN_NOT_FOUND
        execute 'vsplit ' .. filename
    endif
enddef

def BlockInput(cfg: dict<any>): void
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
            SendCharMsg(cfg, current_line)
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
                SendCharMsg(cfg, current_line)
                fake_cursor_position = fake_cursor_position - 1
                PrintFakePrompt(current_line, fake_cursor_position)
            elseif fake_cursor_position > 1
                var current_line_first = current_line[0 : fake_cursor_position - 2]
                var current_line_last = current_line[fake_cursor_position  : -1]
                current_line = current_line_first .. current_line_last
                SendCharMsg(cfg, current_line)
                fake_cursor_position = fake_cursor_position - 1
                PrintFakePrompt(current_line, fake_cursor_position)
            elseif fake_cursor_position == 1
                current_line = current_line[1  : -1]
                SendCharMsg(cfg, current_line)
                fake_cursor_position = fake_cursor_position - 1
                PrintFakePrompt(current_line, fake_cursor_position)
            endif
            clearmatches()
        elseif input == "\<DEL>"
            if fake_cursor_position == 0 
                current_line = current_line[fake_cursor_position + 1 : -1]
                SendCharMsg(cfg, current_line)
                PrintFakePrompt(current_line, fake_cursor_position)
            elseif fake_cursor_position < len(current_line)
                var current_line_first = current_line[0 : fake_cursor_position - 1]
                var current_line_last = current_line[fake_cursor_position + 1 : -1]
                current_line = current_line_first .. current_line_last
                SendCharMsg(cfg, current_line)
                PrintFakePrompt(current_line, fake_cursor_position)
            endif
            clearmatches()
        elseif input == "\<Down>"
            normal j
            redraw
        elseif input == "\<Up>"
            normal k
            redraw
        elseif input == "\<ScrollWheelUp>"
            norm k
            redraw
        elseif input == "\<ScrollWheelDown>"
            norm j
            redraw
        elseif input == "\<ESC>"
            CloseWindow()
            break
        elseif input == "\<C-k>" || input == "\<C-p>"
            norm k
            redraw
        elseif input == "\<C-j>" || input == "\<C-n>"
            norm j
            redraw
        elseif index(values(g_select_keymap), input) >= 0
            if current_line == ":q"
                CloseWindow()
                break
            endif

            var line = getline(".")
            if empty(line)
                continue
            endif

            if exists('g:vim9_fuzzy_yank_enabled') && g:vim9_fuzzy_yank_enabled
                if cfg.mode == "yank"
                    var for_paste = getline('.')
                    var result_lines = split(for_paste, "|")
                    var file_name = cfg.yank_path .. "/" .. result_lines[0]
                    var lines_for_paste = readfile(file_name)
                    if input == g_select_keymap["edit"]
                        CloseWindow()
                        append(line('.'), lines_for_paste)
                    elseif input == g_select_keymap["tabedit"]
                        CloseWindow()
                        system("printf $'\\e]52;c;%s\\a' \"$(base64 <<(</dev/stdin))\" >> /dev/tty", lines_for_paste)
                    endif
                    return
                endif
            endif

            var file_full_path = fnamemodify(GetFullPathFromResult(cfg, line, current_line), ':p')

            CloseWindow()
            if filereadable(file_full_path)
                var mru_msg = {"cmd": "write_mru", "mru_path": cfg.mru_path, "value": file_full_path }
                job_handler.WriteToChannel(cfg.channel, mru_msg, cfg, PrintResult)
                if input == g_select_keymap["edit"]
                    FocusOrOpen(file_full_path)
                elseif input == g_select_keymap["botright_vsp"]
                    execute 'botright vsp ' .. file_full_path
                elseif input == g_select_keymap["tabedit"]
                    execute 'tabedit ' .. file_full_path
                endif
            else
                echom "File: " .. file_full_path .. " cannot be accessed"
            endif
            break
        endif
    endwhile
enddef

def InitProcess(): dict<any>
    if ch_status(g_channel.channel) != "open"
        g_channel = job_handler.StartFinderProcess()
    endif
    return g_channel
enddef

export def StartWindow(...args: list<string>): void
    var mode = args[0]
    var target_dir = ""
    if len(args) > 1
        target_dir = args[1]
    endif
    var channel = InitProcess()
    var cfg = CreateCfg(g_script_dir, GetRootdir(), target_dir, mode, channel)
    InitWindow(cfg)
    try
        BlockInput(cfg)
    catch /^Vim:Interrupt$/
        CloseWindow()
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
