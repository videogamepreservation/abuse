(setq watcom_include "$(%WATCOM)\\h")

(defun app (head tail) (if (null head) tail (cons (car head) (app (cdr head) tail))))

(defun get-string-section (string start end)
  (if (> start end)
      nil
    (cons (elt string start) (get-string-section string (+ start 1) end))))

(defun replace-string-section (string start end replacement)
  (concatenate 'string 
	       (get-string-section string 0 (- start 1))
	       replacement
	       (get-string-section string (+ end 1) (- (length string) 1))))
  

(defun replace..withdd (string)
  (let ((find (search ".." string)))
    (if find
	(replace..withdd  (replace-string-section string find (+ find 1) "dd"))
      string)))

(defun replace.. (string)
  (if (equal outdir ".")
      string
    (let ((find (search ".." string)))
      (if find
	  (replace..withdd (replace-string-section string find (+ find 1) outdir))
	string))))

;(setq platform 'WATCOM)
(setq dirs_to_make '("."))

(defun get_includes (include_list)
  (if (eq platform 'WATCOM) 
      (cons watcom_include include_list)
    include_list))

(defun add_dir_to_make (dir_list new_dir)  
  (if dir_list
      (if (equal (car dir_list) new_dir)
	  nil
	(add_dir_to_make (cdr dir_list) new_dir))
    (setq dirs_to_make (cons new_dir dirs_to_make))))

(defun make_versions ()
  (if (equal platform 'WATCOM)
      '("debug" "opt")
    '("debug" "opt" "profile")))

(defun default_version ()         ;; make will build all targets of this verison  
  (if (equal platform 'LINUX)      ;; linux is debug platform
      "debug"                     
    "opt"))


(defun plat_dot_name (platform)
  (if (eq platform 'SGI)
      ".sgi"
    (if (eq platform 'SUN)
	".sun"
      (if (eq platform 'AIX)
	  ".aix"
	""))))
	

(defun get_version_extension (version platform)
  (concatenate 'string
	       (select version
		       ("debug"   "")
		       ("opt"     "o")
		       ("profile" "p"))
	       (plat_dot_name platform)))
		   
		 
	       


;(setq o_files '("nfserver" "nfclient" "username" "clisp"    "gui"      "transp" 
;		"collide"  "trig"     "property" "lisp"     "cache"    "particle" 
;		"objects"  "server2"  "client"   "extend"   "console"  "ability" 
;		"items"    "dev"      "chars"    "level"    "smallfnt" "automap"
;		"help"     "intsect"  "loader"   "seq"      "points"   "fnt6x13"
;		"morpher"  "menu"     "director" "view"     "config"   "game" 
;		"light"    "devsel"   "crc"      "nnet/nnet")) 


(defun platform_name (plat)
     (select plat
	  ('WATCOM "wat")
	  ('LINUX_X11  "linux_x11")
	  ('LINUX_SVGA  "linux_svga")
	  ('LINUX       "linux")
	  ('AIX    "AIX")
	  ('SGI    "SGI")
	  ('SUN    "SUN")))


(defun slash ()
  (if (equal platform 'WATCOM)
      "\\"
    "/"))

(defun object_extension ()
  (if (equal platform 'WATCOM)
      ".obj"
    ".o"))


(defun version_dir (dir version_name plat) 
;  (let ((platform (if (or (equal plat 'LINUX_SVGA) (equal plat 'LINUX_X11))
;		 "LINUX"
;	       (platform_name plat))))
    (concatenate 'string dir (platform_name plat) (slash) version_name))

(defun make_obj_name (dir basename version_name plat)
  (convert_slashes (concatenate 'string (version_dir dir version_name plat) 
	       (slash) basename (object_extension)) (slash)))

(defun make_file_name ()
  (if (equal platform 'WATCOM)
      "makefile"
    "Makefile"))


(defun link_flags (ver plat)
  (select plat
	  ('LINUX_SVGA (if (equal ver "profile") "-lvga -lm" "-lvga -lm"))
	  ('LINUX_X11 
			   "/lib/libXext.a /lib/libX11.a -lm")
	  ('SUN "-lX11 -lXext -lm")
	  ('SGI "-lX11 -lXext -lm")
	  ('AIX "-lXextSam -lX11 -lXext -lm")
	  ('WATCOM "")))


(defun get_cflags (ver platform)
  (if (equal platform 'WATCOM)
      (concatenate 'string 
		   (if (equal ver "debug")
		       "/zq /d2"
		     "/omaxne /zp1 /zq -DNO_CHECK")
		   " -DMANAGE_MEM")
    (if (equal ver "debug")
	"-g -DMEM_CHECK -DMANAGE_MEM"
      (if (equal ver "profile")
	  (if (eq platform 'SGI)
	      "-O2 -g -p -DMEM_CHECK -DMANAGE_MEM"    ; libcrt0 not supported on IRIX 5.3
	    "-O2 -g -pg -DMEM_CHECK -DMANAGE_MEM")
	"-O2 -DMANAGE_MEM -DNO_CHECK"))))


(defun get_compiler (file)
	(if (equal platform 'WATCOM)
	    "wpp386"
	  "g++"))

(defun line_delimiter ()
  (if (equal platform 'WATCOM)
      "&"
    "\\"))

(defun append_c (filename)
  (concatenate 'string filename ".c"))

(defun get_objs (list ver plat)
  (if (null list) nil
    (let ((x (split_filename (car list) (concatenate 'string outdir (slash)) )))
      (cons (make_obj_name (car x) (cdr x) ver plat) (get_objs (cdr list) ver plat)))))



(defun list_files (list)
  (print "\t" (convert_slashes (car list) (slash)))
  (if (cdr list)
      (progn
	(print " " (line_delimiter) "\n")
	(list_files (cdr list)))
    nil))


(defun isa_target2 (list platform)
  (if list
      (if (eq platform (car list))
	  T
	(isa_target2 (cdr list) platform))
    nil))


(defun isa_target (list platform)
  (if list (isa_target2 list platform)
    T))

(defun add_out_dir (name)
  (if (equal outdir ".")
      name
    (concatenate 'string outdir (slash) name)))

(defun extend_name (name letter version do_it platform)
  (if do_it   
      (concatenate 'string name letter (get_version_extension version platform))
    name))

(defun expand_targets (targets version)

  (if targets
      (let ((plats (car (cdr (cdr (cdr (cdr (cdr (car targets))))))))
	    (extend (if (eq (car (cdr (cdr (car targets)))) 'self) nil T))
	    (target_name (car (car targets)))
	    (rest (cdr (car targets))))

;	(open_file "/dev/tty" "wb" (print (concatenate 'string outdir target_name) "\n"))
;		   (print targets))

	(if (eq platform 'LINUX)	 
	    (if (isa_target plats 'LINUX_X11)
		(cons (cons 'LINUX_X11 (cons (extend_name target_name "x" version extend platform) rest))
		      (if (isa_target plats 'LINUX_SVGA)
			  (cons	(cons 'LINUX_SVGA (cons (extend_name target_name "" version extend platform)
							rest))
				(expand_targets (cdr targets) version))
			(expand_targets (cdr targets) version)))
	      (if (isa_target plats 'LINUX_SVGA)
		  (cons	(cons 'LINUX_SVGA (cons (extend_name target_name "" version extend platform)
						rest))
			(expand_targets (cdr targets) version))
		(expand_targets (cdr targets) version)))
	  (if (isa_target plats platform)
	      (cons
	       (cons platform (cons (extend_name target_name "" version extend platform)  rest))
	       (expand_targets (cdr targets) version)
	       )
	     (expand_targets (cdr targets) version))))))
	    



(defun make_lib_name (target platform version)
  (let ((x (split_filename target (concatenate 'string "." (slash)))))
    (replace.. (convert_slashes (concatenate 'string 
					     (version_dir (car x) version platform) 
					     (slash) (cdr x) (lib_ext)) (slash)))))


(defun get_lib_list (target_list)
  (nth 3 (car target_list)))

(defun list_targets (targets version)
  (if targets
      (let ((platform      (car (car targets)))
	    (target   (nth 1 (car targets)))
	    (ofiles   (nth 2 (car targets)))
	    (libs     (nth 3 (car targets)))
	    (inc      (get_includes (nth 4 (car targets))))
	    (cflags   (nth 5 (car targets))))
	(print " ")
	(if (eq libs 'self)
	    (print (make_lib_name target platform version))
	  (if (eq platform 'WATCOM)
	      (print target ".exe")
	    (print target)))
	(list_targets (cdr targets) version))))

(defun list_depends (file version plat includes cflags libs type)
  (let ((x (split_filename file (concatenate 'string outdir (slash)))))
    (add_dir_to_make dirs_to_make 
		     (concatenate 'string (version_dir (car x) version plat) (slash)))

    (let ((ofile (make_obj_name (car x) (cdr x) version plat)))
      (print ofile " : ")      
      (print file ".c\n")
      (compile_to_object file version plat includes cflags type)
      (for i in (get_depends (concatenate 'string file ".c") (slash) includes) do
	 (print ofile " : " i "\n")))
    ))

  
(defun link_files (outname files version plat)
  (print "\t")
  (if (eq platform 'WATCOM)
      (progn
	(print "wlink @" outname ".lnk\n\n")
	(open_file (concatenate 'string outname ".lnk") "wb"
		   (if (eq version "debug")
		       (print "debug line\n"))
		   (print "system dos4gw\n")
		   (print "option caseexact\n")
		   (print "option map=" outname ".map\n")
		   (print "debug all\n")
		   (print "name " outname ".exe\n")
		   (print "option stack=70k\n")
		   (for i in files do (print "file " i "\n"))
		   )
	)
    (progn
      (print "g++ -o " outname " " (line_delimiter) "\n")
      (list_files files) 
      (print " " (link_flags version plat))
      (print "\n\n")
      )))

(defun get_include_string (list)
  (if list
      (concatenate 'string (car list) (get_include_string (cdr list)))
    "")) 

(defun compile_to_object (file version plat includes cflags type)
  (let ((x (split_filename file (concatenate 'string outdir (slash)))))
    (let ((ofile (make_obj_name (car x) (cdr x) version plat)))
      (if (eq platform 'WATCOM)
	  (progn
	    (make_compile_header includes)
	    (if (eq 'C type)
		(print "\twcc386 ")
	      (print "\twpp386 "))
	    (print file ".c -fo=" ofile " " (get_cflags version plat) " " cflags "\n\n"))
	(progn
	  (if (eq 'C type)
	      (print "\tgcc")
	    (print "\tg++"))
	  (progn
	    (print " -c -o " ofile " " file ".c")
	    (for i in includes do (print " -I" i))
	    (print " " (get_cflags version plat) " " cflags "\n\n" )
	    ))))))

(defun lib_ext ()
  (if (eq platform 'WATCOM) ".lib"
    ".a"))

(defun create_archive (target platform version objs)
  (let ((x (split_filename target (concatenate 'string "." (slash)))))
    (let ((outname (make_lib_name target platform version)))
      (if (eq platform 'WATCOM)	  
	  (let ((link_bat (concatenate 'string (version_dir (car x) version platform)
				    (slash) (cdr x) ".lnk")))
	    (make_dir (concatenate 'string (car x)  (platform_name platform) 
				       (slash) version (slash)))
	    (open_file link_bat "wb"
		       (print outname "\n")
		       (for i in objs do
			    (print "+ " (convert_slashes i (slash)) "\n")))
	    (print "\twlib /n @" link_bat "\n\n"))
	(progn
	  (print "\tar ruvs " outname " " (line_delimiter) "\n")
	  (list_files objs)
	  (print "\n\n"))))))


			    
				    
		       

(defun make_target (list version)
  (if list
      (let ((platform      (car (car list)))
	    (target   (nth 1 (car list)))
	    (ofiles   (nth 2 (car list)))
	    (libs     (nth 3 (car list)))
	    (inc      (get_includes (nth 4 (car list))))
	    (cflags   (nth 5 (car list)))
	    (type     (nth 7 (car list))) )
	(let ((obj_list  (get_objs ofiles version platform)))
	 
	  (print platform "_" target "_" version "_o_files = " (line_delimiter) "\n")
	  (list_files obj_list)
	  (print "\n\n")

	  (make_target (cdr list) version)

	  (if (eq libs 'self)
	      (progn
		(print (make_lib_name target platform version) " : $(" 
		       platform "_" target "_" version "_o_files)\n")
		(create_archive target platform version obj_list))
	    (progn
	      (if  (not (equal "." outdir))
		  (print outdir (slash)))
	      (print target)

	      (if (and (not (eq libs 'self))
		       (eq platform 'WATCOM))
		  (print ".exe"))

	      (print " : $(" platform "_" target "_" version "_o_files)\n")
	      (link_files target (app obj_list (get_lib_files libs platform version)) 
			  version platform)))


	  (for i in ofiles do
	       (list_depends (convert_slashes i (slash)) version platform inc 
			     (if cflags cflags "") libs type ))
	  (print "\n")
	  ))))
	


(defun get_ex_libs (libname plat version)
  (if (eq plat 'WATCOM) 
      (if (equal libname "sound") "e:\\sos\\lib\\sosw1cr.lib" nil)
    (if (eq plat 'LINUX_X11) (if (eq libname "winman") "-lX11 -lXext" nil)
      (if (eq plat 'LINUX_SVGA) (if (eq libname "winman") "-lvga" nil)
	nil))))
		       

(defun get_lib_files (libs plat version)  
  (if libs
      (let ((x (get_ex_libs (cdr (split_filename (car libs) "./")) plat version))
	    (rest (get_lib_files (cdr libs) plat version))
	    (this (make_lib_name (car libs) plat version)))
	(if x 
	    (cons x (cons this rest))
	  (cons  this rest)))
    nil))

    
(defun make_include_string (list)
  (if list
      (concatenate 'string (car list) 
		   (if (cdr list)
		       (concatenate 'string ";" (make_include_string (cdr list)))
		     "")) 
    ""))
     
(defun make_compile_header (include)
  (if (eq platform 'WATCOM)
      (progn
	(print "\tset include=" (make_include_string include) "\n")) nil))
	


(progn
	   (for i in (make_versions) do
		(print i " :")
		(list_targets (expand_targets targets i) i)
		(print "\n\techo Made " i "\n\n")
		)
	   (for i in (make_versions) do
		(make_target (expand_targets targets i) i))


)

(print dirs_to_make)
;(for i in dirs_to_make do (make_dir i))

