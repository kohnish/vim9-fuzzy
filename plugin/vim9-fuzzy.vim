"vim9script
"import autoload "../autoload/window_handler.vim"
"
"command! -nargs=0 Vim9FuzzyFile call window_handler.StartWindow("file")
"command! -nargs=0 Vim9FuzzyMru call window_handler.StartWindow("mru")
"command! -nargs=0 Vim9FuzzyPath call window_handler.StartWindow("path")

command! -nargs=0 Vim9FuzzyFile call window_handler#StartWindow("file")
command! -nargs=0 Vim9FuzzyMru call window_handler#StartWindow("mru")
command! -nargs=0 Vim9FuzzyPath call window_handler#StartWindow("path")
