vim9script

import "./window_handler.vim"

const g_default_executable_path = fnamemodify(resolve(expand('<script>:p')), ':h') .. "/../bin/vim9-fuzzy"

export def StartFinderProcess(): dict<any>
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
    return { "job": job, "channel": job_getchannel(job) }
enddef

var g_flush_timers = {"file": -1}
export def WriteToChannel(ch: channel, msg: dict<any>, ctx: dict<any>, Cb: func): void
    var opt = {
        "callback": (channel: channel, result_msg: dict<any>) => Cb(ctx, result_msg),
    }
    var msg_kind = msg["cmd"]
    if msg_kind == "file"
        var timer = g_flush_timers[msg_kind]
        if !empty(timer_info(timer))
            timer_stop(timer)
        endif
        g_flush_timers[msg_kind] = timer_start(10, (_) => ch_sendexpr(ch, msg, opt))
    else
        ch_sendexpr(ch, msg, opt)
    endif
enddef
