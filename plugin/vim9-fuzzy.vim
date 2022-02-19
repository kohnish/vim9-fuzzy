vim9script

import "./job_handler.vim" as job_handler
import "./window_handler.vim" as window_handler

job_handler.StartFinderProcess()
command -nargs=0 Vim9FuzzyFile window_handler.StartWindow("file")
command -nargs=0 Vim9FuzzyMru window_handler.StartWindow("mru")
command -nargs=0 Vim9FuzzyPath window_handler.StartWindow("path")
