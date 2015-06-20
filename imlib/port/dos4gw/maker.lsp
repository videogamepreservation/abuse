(perm-space)

(setq outdir ".")



(setq targets     
	'(("ipxtest"
	   ("ipxtest")

	   nil
	   ("../../include")
	   "")))


(setq imlib_dir      "../../")
(setq cflags         "")
(compile-file        (concatenate 'string imlib_dir "makemake.lsp"))

