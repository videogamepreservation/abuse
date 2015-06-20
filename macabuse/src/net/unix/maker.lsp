
(perm-space)
(setq outdir ".")

;; appends something to the end of a list
(defun app (head tail) (if (null head) tail (cons (car head) (app (cdr head) tail))))


(setq targets     
	`(("undrv" 
	   ("undrv"       ; the main program
	    "tcpip"       ; unix tcpip related net interface
	    "netdrv"      ; engine/driver interface 
	    "gserver"     ; game server
	    "gclient"     ; game client
	    "fileman"     ; file manager
	    )
	   nil
	   ("../inc")
	   "")))

(setq imlib_dir      "../../../../imlib/")
(setq cflags         "")
(compile-file        (concatenate 'string imlib_dir "makemake.lsp"))

