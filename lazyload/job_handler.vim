vim9script

import "./window_handler.vim"

var g_channel: channel
const g_default_executable_path = fnamemodify(resolve(expand('<script>:p')), ':h') .. "/../bin/vim9-fuzzy"

export def StartFinderProcess(): channel
    var job_opt = {
        "out_mode": "lsp",
        "in_mode": "lsp",
        "noblock": 1,
    }
    var executable = ""
    if !exists('g:vim9_fuzzy_exe_path')
        executable = g_default_executable_path
        if has("win64") || has("win32") || has("win16") || has("win32unix")
            executable = g_default_executable_path .. ".exe"
        endif
    else
        executable = g:vim9_fuzzy_exe_path
    endif
    var job = job_start([executable], job_opt)
    g_channel = job_getchannel(job)
    return g_channel
enddef

export def WriteToChannel(msg: dict<any>, ctx: dict<any>, Cb: func): void
    var opt = {
        "callback": (channel: channel, result_msg: dict<any>) => Cb(ctx, result_msg),
    }
    ch_sendexpr(g_channel, msg, opt)
enddef

