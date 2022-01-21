vim9script

import "./job_handler.vim" as job_handler
import "./window_handler.vim" as window_handler

var g_initialised = false

def Main(): void
    if !g_initialised
        g_initialised = true
        job_handler.StartFinderProcess()
    else
        echomsg "Already initialised"
    endif
enddef

def StartWindow(mode: string): void
    call window_handler.StartWindow(mode)
enddef



if exists('g:vim9_fuzzy_disabled') && g:vim9_fuzzy_disabled
    finish
else
    Main()
    command! -nargs=0 Vim9FuzzyFile StartWindow("file")
    command! -nargs=0 Vim9FuzzyMru StartWindow("mru")
    command! -nargs=0 Vim9FuzzyPath StartWindow("path")
endif
