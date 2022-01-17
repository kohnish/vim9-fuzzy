vim9script

import "./window_handler.vim" as window_handler

g:job = "null"
g:channel = "null"

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
    var executable = fnamemodify(resolve(expand('<stack>:p')), ':h') .. "/../bin/vim9-fuzzy"
    if has("win64") || has("win32") || has("win16")
        executable = executable .. ".exe"
    endif
    g:job = job_start([executable], job_opt)
    g:channel = job_getchannel(g:job)
enddef


export def WriteToChannel(msg: dict<any>): void
    ch_sendexpr(g:channel, msg)
enddef

