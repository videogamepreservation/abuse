(perm-space)

(setq outdir ".")

;; appends something to the end of a list
(defun app (head tail) (if (null head) tail (cons (car head) (app (cdr head) tail))))

(setq netfile (select platform
		      ('LINUX "src/net/mac/tcpip")
		      ('SGI   "src/net/mac/tcpip")
		      ('AIX   "src/net/mac/tcpip")
                      ('WATCOM "src/net/dos4gw/ipx_dud")
			))

(setq targets     
	`(("game"     
	   ,(cons netfile 
		  '("src/specache" "../zebra_imlib/sprite" "src/netcfg"  "src/text_gui"
		    "src/innet"                      "src/net/unix/gserver"  
		    "src/net/unix/gclient"           "src/net/unix/fileman"            "src/net/sock"
		    "src/chat"      "src/endgame"    "src/setup"       "src/version"   "src/loadgame"   "src/profile"
		    "src/cop"        "src/statbar"   "src/compiled"    "src/ant"       "src/sensor" 
		    "src/lisp_opt"   "src/demo"      "src/lcache"      "src/lisp_gc"
		    "src/nfclient"   "src/username"  "src/clisp"       "src/gui" 
		    "src/transp"     "src/collide"   "src/trig"        "src/property" 
		    "src/lisp"       "src/cache"     "src/particle"    "src/objects"
		    "src/extend"     "src/console"   "src/ability" 
		    "src/items"      "src/dev"       "src/chars"        "src/level"    "src/smallfnt" "src/automap" 
		    "src/help"       "src/intsect"   "src/loader2"       "src/seq"      
		    "src/points"     "src/fnt6x13"   "src/morpher"      "src/menu"     "src/director" "src/view" 
		    "src/config"     "src/game"      "src/light"        "src/devsel" 
		    "src/crc"        "src/gamma"     "src/language"))

	   ("../zebra_imlib/image" "../zebra_imlib/winman" "../zebra_imlib/gui" "../zebra_imlib/dir"
	    "../zebra_imlib/time" "../zebra_imlib/sound" "../zebra_imlib/joy"   "../zebra_imlib/packet")
	   ("inc" "../zebra_imlib/include" "src/net/inc" "src/net/unix" "src/net/inc")
	   "")))

(setq imlib_dir      "../zebra_imlib/")
(setq cflags         "")
(compile-file        (concatenate 'string imlib_dir "makemake.lsp"))

