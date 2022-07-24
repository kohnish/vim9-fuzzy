vim9script
import autoload "../lazyload/window_handler.vim"

command! -nargs=0 Vim9FuzzyFile window_handler.StartWindow("file")
command! -nargs=0 Vim9FuzzyMru window_handler.StartWindow("mru")
command! -nargs=0 Vim9FuzzyPath window_handler.StartWindow("path")
