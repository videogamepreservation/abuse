(perm-space)
(setq c_flags        "-DMANAGE_MEM")
(setq outdir ".")


(setq targets '(("image"                                    ; target name
		 ("filter"   "gifread" "globals" "image"  "linked" "input"
		  "mdlread"  "palette" "ppmread" "include" "xwdread" 
		  "fonts"    "decoder" "loader"  "glread" "texture" "specs" 
		  "supmorph" "image24" "pcxread" "timage" "jmalloc" 
		  "jrand"    "lbmread" "keys"    "dprint" "status"  "visobj")
		 self
		 ("include") "" (LINUX_X11 LINUX_SVGA AIX SUN SGI WATCOM))
		("packet" ("packet") self ("include") "" (LINUX_X11 LINUX_SVGA AIX SUN SGI WATCOM))

		("gui" ("pmenu" "scroller" "filesel" "tools" "guistat") self ("include"))
		("sound"  ("port/sgi/sound" "readwav") self ("include") "" (LINUX_X11 LINUX_SVGA))
		("net"    ("port/unix/jnet") 
		 self ("include") "" (LINUX_X11 LINUX_SVGA AIX SUN SGI))
		("dir"    ("port/unix/jdir") self ("include") "" (LINUX_X11 LINUX_SVGA AIX SUN SGI))
		("sound"    ("port/unix/sound")          self ("include") "" (AIX SUN))
		("sound"    ("port/sgi/sound" "readwav") self  ("include") "" (SGI))
		("joy"    ("port/unix/joystick")         self ("include") "" (AIX SUN SGI LINUX_X11 LINUX_SVGA))
		("winman" ("port/x11/video"
			   "port/x11/mouse"
			   "port/x11/event"
			   "jwindow")        self ("include") "" (LINUX_X11 AIX SUN))

		("winman" ("port/x11/video"
			   "port/x11/mouse"
			   "port/x11/event"
			   "jwindow")        self ("include") "" (SGI))

		("winman" ("port/svga/video"
			   "port/svga/mouse"
			   "port/svga/event"
			   "jwindow")        self ("include") "" (LINUX_SVGA))

		("time"   ("port/unix/timing") self ("include") "" (LINUX_X11 LINUX_SVGA AIX SUN))
		("time"   ("port/sgi/timing") self ("include") "" (SGI))
		    
		    


		("sound" ("port/dos4gw/sound" "readwav" "port/dos4gw/profile" ) 
                          self ("include" "c:\\sos\\include")  "" (WATCOM))
		("net"    ("port/dos4gw/jnet" "port/dos4gw/ipx"
			   "port/dos4gw/bwtcp") self ("include")  "" (WATCOM))

		("dir"    ("port/dos4gw/jdir") self ("include")  "" (WATCOM))
		("winman" ("port/dos4gw/video"
			   "port/dos4gw/mouse"
			   "port/dos4gw/event"
			   "port/dos4gw/doscall"
			   "jwindow")        self ("include") "" (WATCOM))
		("joy"    ("port/dos4gw/joystick") self ("include")  "" (WATCOM))
		("time"   ("port/dos4gw/timing") self ("include")  "" (WATCOM))


		))
					

(setq imlib_dir      "./")
(setq cflags         "")

(compile-file        (concatenate 'string imlib_dir "makemake.lsp"))








