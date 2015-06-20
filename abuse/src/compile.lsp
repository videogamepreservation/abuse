(setq __gen 0)
(setq indent 0)

(defun gen_sym (base)
  (concatenate 'string (symbol-name base) (digstr (setq __gen (+ __gen 1)) 5)))

(defun print_indent (level)
  (if (eq level 0)
      nil
    (progn 
      (print " ")
      (print_indent (- level 1)))))

(defun pi ()
  (print_indent indent))

(defun i+ () (setq indent (+ indent 2)))
(defun i- () (setq indent (- indent 2)))


(defun compile-if (? x y)
  (compile ?)
  (pi) (print "if (pop())\n") 
  (pi) (print "{\n")          (i+)
  (compile x)                 (i-)
  (pi) (print "} else\n")     
  (pi) (print "{\n")          (i+)
  (compile y)                 (i-)
  (pi) (print "}\n")
  )


(defun


(defun compile (val)
  (if (listp val
	     (select (car list)
		     ('if (compile-if 









(print (if a 1 2))

push(a)
if (pop())
  push(1)
else push(2);

print(pop())
