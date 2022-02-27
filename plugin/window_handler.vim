vim9script

import "./job_handler.vim" as job_handler

# ToDo: stop using globals
var g_search_window_name = ""
var g_root_dir = ""
var g_list_cmd = ""
var g_current_line = ""
var g_mru_path = ""
var g_script_dir = expand('<stack>:p:h')

export def PrintResult(json_msg: dict<any>): void
    deletebufline(g_search_window_name, 1, "$")
    if len(json_msg["result"]) != 0
        clearmatches()
        highlight matched_str_colour guifg=red ctermfg=red term=bold gui=bold
        var counter = 0
        var line_len = len(g_current_line)
        matchadd("matched_str_colour", g_current_line)
        # ToDo: Properly highlight fuzzy matched characters
        for c in split(g_current_line, '\zs')
            if counter != line_len - 1
                matchadd("matched_str_colour", g_current_line[counter : counter + 1])
            else
                matchadd("matched_str_colour", g_current_line[counter - 1 : counter])
            endif
            counter = counter + 1
        endfor
        setbufline(g_search_window_name, 1, json_msg["result"])
    endif
    redraw
enddef


def InitPrompt(): void
    echon "\r\r"
    echon ''
    echohl Constant | echon '>> ' | echohl NONE
enddef


def InitWindow(mode: string): void
    noautocmd keepalt keepjumps botright split \ Vim9 Fuzzy
    g_search_window_name = bufname()
    resize 20

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
    if exists('g:vim9_fuzzy_proj_dir') && g:vim9_fuzzy_proj_dir != "/" && getcwd() != "/"
        g_root_dir = g:vim9_fuzzy_proj_dir
    else
        g_root_dir = getcwd()
    endif

    var git_cmd = "git"
    var rg_cmd = "rg"
    if has("win64") || has("win32") || has("win16")
        git_cmd = git_cmd .. ".exe"
        rg_cmd = rg_cmd .. ".exe"
    endif
    var git_dir = g_root_dir .. "/.git"
    if filereadable(git_dir) || isdirectory(git_dir)
        g_list_cmd = git_cmd .. " ls-files " .. g_root_dir .. " --full-name"
    else
        g_list_cmd = rg_cmd .. " --files --hidden --max-depth 5"
    endif

    if exists('g:vim9_fuzzy_mru_path')
        g_mru_path = g:vim9_fuzzy_mru_path
    else
        g_mru_path = g_script_dir .. "/../mru"
    endif

    if mode == "file"
        job_handler.WriteToChannel({"cmd": "init_file", "root_dir": g_root_dir, "list_cmd": g_list_cmd})
    elseif mode == "path"
        job_handler.WriteToChannel({"cmd": "init_path", "root_dir": g_root_dir, "list_cmd": g_list_cmd})
    elseif mode == "mru"
        job_handler.WriteToChannel({"cmd": "init_mru", "mru_path": g_mru_path})
    endif

    redraw
enddef


def CloseWindow(): void
     bdelete
     echohl Normal | echon '' | echohl NONE
     redraw
enddef


def SendCharMsg(mode: string, msg: string): void
    if mode == "file"
        if len(msg) == 0
            var msg2send = {"cmd": "init_file", "root_dir": g_root_dir, "list_cmd": g_list_cmd}
            job_handler.WriteToChannel(msg2send)
        else
            var msg2send = {"cmd": "file", "value": msg, "root_dir": g_root_dir, "mru_path": g_mru_path}
            job_handler.WriteToChannel(msg2send)
        endif
    elseif mode == "path"
        if len(msg) == 0
            var msg2send = {"cmd": "init_path", "root_dir": g_root_dir, "list_cmd": g_list_cmd}
            job_handler.WriteToChannel(msg2send)
        else
            var msg2send = {"cmd": "file", "value": msg, "root_dir": g_root_dir, "mru_path": g_mru_path}
            job_handler.WriteToChannel(msg2send)
        endif
    elseif mode == "mru"
        if len(msg) == 0
            var msg2send = {"cmd": "init_mru", "mru_path": g_mru_path}
            job_handler.WriteToChannel(msg2send)
        else
            var msg2send = {"cmd": "mru", "value": msg, "mru_path": g_mru_path}
            job_handler.WriteToChannel(msg2send)
        endif
    endif
enddef

def PrintFakePrompt(line: string, cursor_pos: number): void
    InitPrompt()
    if cursor_pos > 0
        echohl Normal | echon g_current_line[0 : cursor_pos - 1] | echohl NONE
    endif
    if cursor_pos != len(line)
        echohl Cursor | echon g_current_line[cursor_pos] | echohl NONE
    endif
    echohl Normal | echon g_current_line[cursor_pos + 1 : -1] | echohl NONE
    if cursor_pos == len(line)
        echohl Cursor | echon ' ' | echohl NONE
    else
        echohl Normal | echon ' ' | echohl NONE
    endif
    redraw
enddef

def FocusOnNonTerminal(filename: string): bool
    for buf in getbufinfo()
        if buf.loaded && len(buf.windows) > 0 && getbufvar(buf.bufnr, '&buftype') != "terminal"
            win_gotoid(buf.windows[0])
            return true
        endif
    endfor
    return false
enddef

def FocusOrOpen(filename: string): void
    var buffers = getbufinfo()
    for buf in buffers
        if buf.loaded && buf.name == filename && len(buf.windows) > 0
            win_gotoid(buf.windows[0])
        endif
    endfor
    if &buftype == "terminal" && !FocusOnNonTerminal(filename)
        execute 'tabnew ' .. filename
    elseif &modified
        execute 'vsplit ' .. filename
    else
        execute "edit " .. filename
    endif
enddef


def BlockInput(mode: string): void
    g_current_line = ""
    var fake_cursor_position = 0
    while true
        var input = getcharstr()
        var input_num = char2nr(input)

        if input_num < 127 && input_num > 31
            if fake_cursor_position < len(g_current_line) && len(g_current_line) > 0
                if fake_cursor_position == 0
                    g_current_line = input .. g_current_line[0 : fake_cursor_position] .. g_current_line[fake_cursor_position + 1 : -1]
                    fake_cursor_position = fake_cursor_position + 1
                    PrintFakePrompt(g_current_line, fake_cursor_position)
                else
                    g_current_line = g_current_line[0 : fake_cursor_position - 1] .. input .. g_current_line[fake_cursor_position : -1]
                    fake_cursor_position = fake_cursor_position + 1
                    PrintFakePrompt(g_current_line, fake_cursor_position)
                endif
            elseif fake_cursor_position > 0
                g_current_line = g_current_line[0 : fake_cursor_position] .. input .. g_current_line[fake_cursor_position + 1 : -1]
                fake_cursor_position = fake_cursor_position + 1
                PrintFakePrompt(g_current_line, fake_cursor_position)
            else
                if g_current_line == ""
                    g_current_line = input .. g_current_line
                    fake_cursor_position = fake_cursor_position + 1
                    PrintFakePrompt(g_current_line, fake_cursor_position)
                else
                    g_current_line = input .. g_current_line
                    PrintFakePrompt(g_current_line, fake_cursor_position)
                endif
            endif
            SendCharMsg(mode, g_current_line)
        elseif input == "\<Left>"
            if fake_cursor_position > 0
                fake_cursor_position = fake_cursor_position - 1
                PrintFakePrompt(g_current_line, fake_cursor_position)
            endif
        elseif input == "\<HOME>"
            fake_cursor_position = 0
            PrintFakePrompt(g_current_line, fake_cursor_position)
        elseif input == "\<END>"
            fake_cursor_position = len(g_current_line)
            PrintFakePrompt(g_current_line, fake_cursor_position)
        elseif input == "\<Right>"
            if fake_cursor_position <= len(g_current_line) - 1
                InitPrompt()
                fake_cursor_position = fake_cursor_position + 1
                PrintFakePrompt(g_current_line, fake_cursor_position)
            endif
        elseif input == "\<BS>"
            if fake_cursor_position < 1
                continue
            elseif fake_cursor_position == len(g_current_line)
                g_current_line = g_current_line[0 : -2]
                SendCharMsg(mode, g_current_line)
                fake_cursor_position = fake_cursor_position - 1
                PrintFakePrompt(g_current_line, fake_cursor_position)
            elseif fake_cursor_position > 1
                var current_line_first = g_current_line[0 : fake_cursor_position - 2]
                var current_line_last = g_current_line[fake_cursor_position  : -1]
                g_current_line = current_line_first .. current_line_last
                SendCharMsg(mode, g_current_line)
                fake_cursor_position = fake_cursor_position - 1
                PrintFakePrompt(g_current_line, fake_cursor_position)
            elseif fake_cursor_position == 1
                g_current_line = g_current_line[1  : -1]
                SendCharMsg(mode, g_current_line)
                fake_cursor_position = fake_cursor_position - 1
                PrintFakePrompt(g_current_line, fake_cursor_position)
            endif
            clearmatches()
        elseif input == "\<DEL>"
            if fake_cursor_position == 0 
                g_current_line = g_current_line[fake_cursor_position + 1 : -1]
                SendCharMsg(mode, g_current_line)
                PrintFakePrompt(g_current_line, fake_cursor_position)
            elseif fake_cursor_position < len(g_current_line)
                var current_line_first = g_current_line[0 : fake_cursor_position - 1]
                var current_line_last = g_current_line[fake_cursor_position + 1 : -1]
                g_current_line = current_line_first .. current_line_last
                SendCharMsg(mode, g_current_line)
                PrintFakePrompt(g_current_line, fake_cursor_position)
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
        elseif input == "\<C-k>"
            norm k
            redraw
        elseif input == "\<C-j>"
            norm j
            redraw
        elseif input == "\<CR>" || input == "\<C-t>" || input == "\<C-]>"
            if g_current_line == ":q"
                CloseWindow()
                break
            endif
            var line = getline(".")
            var file_full_path = g_root_dir .. "/" .. line
            if filereadable(g_current_line)
                file_full_path = g_current_line
            elseif mode == "mru"
                file_full_path = line
            endif
            CloseWindow()
            if filereadable(file_full_path)
                var mru_msg = {"cmd": "write_mru", "mru_path": g_mru_path, "value": file_full_path }
                job_handler.WriteToChannel(mru_msg)
                if input == "\<CR>"
                    FocusOrOpen(file_full_path)
                elseif input == "\<C-]>"
                    execute 'botright vsp ' .. file_full_path
                elseif input == "\<C-t>"
                    execute 'tabedit ' .. file_full_path
                endif
            else
                echom "File: " .. file_full_path .. " cannot be accessed"
            endif
            break
        endif
    endwhile
enddef


export def StartWindow(mode: string): void
    InitWindow(mode)
    try
        BlockInput(mode)
    catch /^Vim:Interrupt$/
        CloseWindow()
    endtry
enddef
