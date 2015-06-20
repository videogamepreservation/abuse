(perm-space)

(setq outdir "/tmp/")

(defun imlib (filename) (concatenate 'string "../imlib/" filename))
(defun i4 (filename) (concatenate 'string "../i4/" filename))
(defun x (y) (cons y "hI"))

(defun list_imlib (list)
  (if list
      (cons (imlib (car list)) (list_imlib (cdr list)))))

(setq platform-files 
      (select platform
	      ('LINUX			    
               (print (mapcar 'x '("hi" "there")))
               (list_imlib
                       '("port/unix/jdir"
                         "port/unix/joystick"
                         "port/x11/video"
                         "port/x11/mouse"
                         "port/x11/event"
                         "port/unix/timing"
                         )))

	      ('SGI
               (list_imlib
                       '("port/sgi/sound"
                         "port/unix/joystick"
                         "port/x11/video"
                         "port/x11/mouse"
                         "port/x11/event"
                         "port/sgi/timing"
                         "port/unix/jdir"
                         )))
              ('WATCOM 
               (list_imlib
                       '("port/svga/video"
                         "port/svga/mouse"
                         "port/svga/event"
                         "port/dos4gw/sound"
                         "port/dos4gw/profile"
                         "port/dos4gw/jdir"
                         "port/dos4gw/video"
                         "port/dos4gw/mouse"
                         "port/dos4gw/event"
                         "port/dos4gw/doscall"
                         "port/dos4gw/joystick"
                         "port/dos4gw/timing"
                         )))
              ))
              



(print "Generating makefile for : ")
(print platform)

(setq executable_name            "game")
(setq c_files_used    (nconc 
		             platform-files
                             (list_imlib 
                                     '("filter"   "gifread" "globals" "image"  "linked" "input"
                                       "mdlread"  "palette" "ppmread" "sprite" "include" "xwdread" 
                                       "fonts"    "decoder" "loader"  "glread" "texture" "specs" 
                                       "supmorph" "image24" "pcxread" "timage" "jmalloc" 
                                       "jrand"    "lbmread" "keys"    "dprint" "status"  "visobj"
                                       "readwav"  "packet"
                                       "pmenu" "scroller" "filesel" "tools" "guistat"
                                       "jwindow"
                                       ))
                             '("src/netcfg"  
                               "src/text_gui"
                               "src/innet"                      
                               "src/net/unix/gserver"  
                               "src/net/unix/gclient"           
                               "src/net/unix/fileman"            
                               "src/net/sock"
                               "src/chat"      
                               "src/endgame"    
                               "src/setup"       
                               "src/version"    
                               "src/loadgame"  
                               "src/profile"
                               "src/cop"        
                               "src/statbar"   
                               "src/compiled"    
                               "src/ant"       
                               "src/sensor" 
                               "src/lisp_opt"   
                               "src/demo"      
                               "src/lcache"      
                               "src/lisp_gc"
                               "src/nfclient"   
                               "src/username"  
                               "src/clisp"       
                               "src/gui" 
                               "src/transp"     
                               "src/collide"   
                               "src/trig"        
                               "src/property" 
                               "src/lisp"       
                               "src/cache"     
                               "src/particle"    
                               "src/objects"
                               "src/extend"     
                               "src/console"   
                               "src/ability" 
                               "src/items"      
                               "src/dev"       
                               "src/chars"        
                               "src/level"    
                               "src/smallfnt"   
                               "src/automap" 
                               "src/help"       
                               "src/intsect"   
                               "src/loader"       
                               "src/seq"      
                               "src/points"     
                               "src/fnt6x13"   
                               "src/morpher"      
                               "src/menu"     
                               "src/director" 
                               "src/view" 
                               "src/config"     
                               "src/game"      
                               "src/light"        
                               "src/devsel" 
                               "src/crc"        
                               "src/gamma"     
                               "src/language")))



(setq inc_directories (list (imlib "include") "inc" ))
(setq libraries_used ())
			   


(setq targets     
	`((,executable_name
	   ,c_files_used
	   ,libraries_used
	   ,inc_directories
	   "")))

(setq cflags         "")
(trace)
(compile-file        (i4 "lisp/makemake.lsp"))