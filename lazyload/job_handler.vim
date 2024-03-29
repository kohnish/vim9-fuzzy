vim9script

import "./window_handler.vim"

const g_default_executable_path = fnamemodify(resolve(expand('<script>:p')), ':h') .. "/../bin/vim9-fuzzy"

def PrintError(channel: channel, msg: string): void
    echom msg
enddef

export def StartFinderProcess(): dict<any>
    var job_opt = {
        "out_mode": "lsp",
        "in_mode": "lsp",
        "env": { "UV_THREADPOOL_SIZE": 1 },
        "err_cb": PrintError,
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

def SendWhenOnline(msg_kind: string, ch: channel, msg: dict<any>, opt: dict<any>): void
    if ch_status(ch) == "open"
        ch_sendexpr(ch, msg, opt)
    else
        g_flush_timers[msg_kind] = timer_start(100, (_) => SendWhenOnline(msg_kind, ch, msg, opt))
    endif
enddef

var g_flush_timers = {}
export def WriteToChannel(ch: channel, msg: dict<any>, ctx: dict<any>, Cb: func): void
    var opt = {
        "callback": (channel: channel, result_msg: dict<any>) => Cb(ctx, result_msg),
    }
    var msg_kind = msg["cmd"]
    var timer = get(g_flush_timers, msg_kind, -1)
    if !empty(timer_info(timer))
        timer_stop(timer)
    endif
    g_flush_timers[msg_kind] = timer_start(100, (_) => SendWhenOnline(msg_kind, ch, msg, opt))
enddef
