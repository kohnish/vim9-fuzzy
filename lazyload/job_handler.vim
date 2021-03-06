vim9script

import "./window_handler.vim"

var g_channel: channel
var default_executable_path = fnamemodify(resolve(expand('<script>:p')), ':h') .. "/../bin/vim9-fuzzy"

def HandleStdout(channel: channel, msg: string): void
    try
        var json_msg = json_decode(msg)
        window_handler.PrintResult(json_msg)
    catch
        ## Solve this issue...
        #echom msg
    endtry
enddef


def HandleStdErr(channel: channel, msg: string): void
    echom "Native process error: " .. msg
enddef


export def StartFinderProcess(): void
    var job_opt = {
        "out_cb": HandleStdout,
        "out_mode": "raw",
        "in_mode": "json",
    }
    var executable = ""
    if !exists('g:vim9_fuzzy_exe_path')
        executable = default_executable_path
        if has("win64") || has("win32") || has("win16") || has("win32unix")
            executable = default_executable_path .. ".exe"
        endif
    else
        executable = g:vim9_fuzzy_exe_path
    endif
    var job = job_start([executable], job_opt)
    g_channel = job_getchannel(job)
enddef


export def WriteToChannel(msg: dict<any>): void
    ch_sendexpr(g_channel, msg)
enddef

