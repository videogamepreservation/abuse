
;; If you aren't sure what to translate, give me call
;;  Billy is doing pig-lating :)

(perm-space)

(select platform
        ('SGI (let ((cwd (get_cwd)))
                (chdir (getenv "HOME"))
                (system "ls")
                (print (concatenate 'string "tar -xvf " cwd "/linux/abuse.tar"))
                (system (concatenate 'string "cd ~/ ; tar -xvf " cwd "/linux/abuse.tar"))
                (print "Type cd ~/abuse ; abuse <ENTER> to begin")
                (quit)))
        ('LINUX (let ((cwd (get_cwd)))
                  (chdir (getenv "HOME"))
                  (system "ls")
                  (print (concatenate 'string "tar -xvf " cwd "/linux/abuse.tar"))
                  (system (concatenate 'string "cd ~/ ; tar -xvf " cwd "/linux/abuse.tar"))
                  (print "Type cd ~/abuse ; abuse <ENTER> to begin")
                  (quit))))



(do ((ok nil nil))
    ((eq ok T) nil)
    (select (nice_menu "Abuse" "Select language" '("English" "Franáais" "Deutsch")) ; "Pig Latin"))
            (-1 (quit))                    ;; can't ask to quit, because we don't know the language yet
            (0 (setq lang 'english)
               (setq title       "Abuse Installation")
               (setq path-prompt "Enter path to install to")
               (setq bad-path  '("You entered a bad path name"
                                 "Press any key to re-enter, ESC to quit"))
               (setq quit-title  "Quit?")
               (setq yes-key     "Y")
               (setq no-key      "N")
               (setq quit-msg    "Do you you want to quit? (Y/N)")
               (setq make-dir?  '("Directory does not exsist."
                                  "Do you want to create it? (Y/N)"))
               (setq mkdir-failed "Unable to create directory, retry? (Y/N)")
               (setq nospace-dos  '("Not enough disk space available for this drive"
                                    "You need at least 10.5MB free"
                                    "Would you like to try another drive? (Y/N)"))
               (setq next-disk  '("Insert this disk into disk drive and press SPACE BAR"
                                  "to continue.  Press ESC to quit."))
               (setq copy-title  "Copying files")
               (setq start-msg   "Type abuse <ENTER> to begin")
               (setq path_not_valid '("The pathname you entered is not valid, continue? (Y/N)"))
               (setq ok T)
               )


            (1 (setq lang 'french)
               (setq title       "Installation d'Abuse")
               (setq path-prompt "Entrez le chemin sur lequel installer")
               (setq bad-path  '("Le nom du chemin est incorrect"
                                 "Appuyez sur une touche pour entrer a nouveau, sur ECHAP pour sortir"))
               (setq quit-title  "Sortir ?")
               (setq yes-key     "O")
               (setq no-key      "N")
               (setq quit-msg    "Voulez-vous sortir ? (O/N)")
               (setq make-dir?  '("Ce rÇpertoire n'existe pas."
                                  "Voulez-vous le crÇer ? (O/N)"))
               (setq mkdir-failed "Impossible de crÇer le rÇpertoire, voulez-vous rÇessayer ? (O/N)")
               (setq nospace-dos  '("Espace disque dur insuffisant pour ce lecteur"
                                    "Vous devez avoir au moins 10,5 Mo disponibles"
                                    "Voulez-vous essayer sur un autre lecteur ? (O/N)"))
               (setq next-disk  '("InsÇrez la disquette dans le lecteur et appuyez sur la BARRE D'ESPACE"
                                  "pour continuer.  Appuyez sur ECHAP pour sortir."))
               (setq copy-title  "En train de copier les fichiers")
               (setq start-msg   "Tapez abuse <ENTREE> pour commencer")
               (setq path_not_valid '("Le nom du chemin est incorrect, voulez-vous continuer ? (O/N)"))
               (setq ok T)
               )

            (2 (setq lang 'german)
               (setq title       "Abuse Installation")
               (setq path-prompt "Geben Sie den Installations-Pfadnamen ein")
               (setq bad-path  '("Pfadname ungÅltig"
                                 "Beliebige Taste zur erneuten Eingabe drÅcken, ESC, um abzubrechen"))
               (setq quit-title  "Abbrechen?")
               (setq yes-key     "J")
               (setq no-key      "N")
               (setq quit-msg    "Wollen Sie abbrechen? (J/N)")
               (setq make-dir?  '("Verzeichnis existiert nicht."
                                  "Wollen Sie das Verzeichnis anlegen? (J/N)"))
               (setq mkdir-failed "Verzeichnis kann nicht angelegt werden, erneut versuchen? (J/N)")
               (setq nospace-dos  '("Nicht genug Festplattenspeicher fÅr dieses Laufwerk."
                                    "Sie benîtigen mindestens 10,5 MB."
                                    "Mîchten Sie es auf einem anderen Laufwerk versuchen?(J/N)"))
               (setq next-disk  '("Legen Sie die Diskette in das Laufwerk ein, und drÅcken Sie die LEERTASTE,"
                                  "um weiterzumachen oder ESC, um abzubrechen."))
               (setq copy-title  "Dateien kopieren")
               (setq start-msg   "Tippen Sie abuse <EINGABE>, um mit dem Spiel zu beginnen.")
               (setq path_not_valid '("UngÅltiger Pfadname, fortfahren? (J/N)"))
               (setq ok T)
               )

            (3 (setq lang 'pig_latin
               (setq title       "Abuse Installation")
               (setq path-prompt "Enter path to install to")
               (setq bad-path  '("You entered a bad path name"
                                 "Press any key to re-enter, ESC to quit"))
               (setq quit-title  "Quit?")
               (setq yes-key     "Y")
               (setq no-key      "N")
               (setq quit-msg    "Do you want to quit? (Y/N)")
               (setq make-dir?  '("Directory does not exsist."
                                  "Do you want to create it? (Y/N)"))
               (setq mkdir-failed "Unable to create directory, retry? (Y/N)")
               (setq nospace-dos  '("Not enough disk space available for this drive"
                                    "You need at least 10.5MB free"
                                    "Would you like to try another drive? (Y/N)"))
               (setq next-disk  '("Insert this disk into disk drive and press SPACE BAR"
                                  "to continue.  Press ESC to quit."))
               (setq copy-title  "Copying files")
               (setq start-msg   "Type abuse <ENTER> to begin")
               (setq path_not_valid '("The pathname you entered is not valid, continue? (Y/N)"))
               (setq ok T)
               )

            )))


(defun quit-install ()
  (if (show_yes_no quit-title quit-msg yes-key no-key)
      (quit)))

(defun slash ()
  (select platform
          ('WATCOM   "\\")
          ('UNIX "/")))


(defun append-slash (path)
  (if (equal (schar path (- (length path) 1)) (schar (slash) 0))
      path
    (concatenate 'string path (slash))))

(defun hack-string (x1 x2 st)
  (if (<= x1 x2)
      (cons (schar st x1) (hack-string (+ x1 1) x2 st))
    nil))

(defun remove-slash (path)
  (if (equal (schar path (- (length path) 1)) (schar (slash) 0))
      (concatenate 'string (hack-string 0 (- (length path) 2) path))
   path))


(defun copy-file (disk-name path)
  (do ((ok nil nil))
      ((eq ok T) nil)
      (if (file_exsist (concatenate 'string disk-name ".dat"))
          (if (nice_copy copy-title (concatenate 'string disk-name ".dat")
			 (concatenate 'string path disk-name ".exe"))
              (setq ok T))

	(if (not (show_yes_no title (cons disk-name next-disk) " " ESC_string))
	    (quit))))
  T)


(defun install (path)
  (select platform
          ('WATCOM
           (if (< (K_avail path) 10500)     ; need ~8MB for game and and ~2.5MB extra for install
               (if (show_yes_no title (cons install-path nospace-dos) yes-key no-key)
                   nil
                 (quit))
             (if (and (copy-file "disk1" path)
                      (copy-file "disk2" path)
                      (copy-file "disk3" path))
                 (progn
                   (go_there path)
                   (system "disk1.exe")
                   (system "del disk1.exe")
                   (system "disk2.exe")
                   (system "del disk2.exe")
                   (system "disk3.exe")
                   (system "del disk3.exe")
                   T)
               nil)))
          ('UNIX
           (print (K_avail path))
           (if (< (K_avail path) 8500)
               (if (show_yes_no title (cons install-path nospace-unix) yes-key no-key)
                   nil
                 (quit))
             (let ((cur-dir (get_cwd)))
               (system (concatenate 'string "cd " path))
               (system (concatenate 'string "tar -xvf " cur-dir " abuse.tar"))
               T)))))




(defun lstring (x st)
  (if (< x (length st))
      (progn (print (schar st x))
             (lstring (+ x 1) st))))

(defun go_there (path)
  (select platform
          ('WATCOM
           (if (and (< 2 (length path)) (eq (schar path 1) #\:))
                       (system (concatenate 'string (list (schar path 0) #\:))))
                   (chdir (remove-slash path)))
          ('UNIX (chdir path))))

(defun ok_pathchar (char pos)
  (or (and (>= (char-code char) (char-code #\a))
           (<= (char-code char) (char-code #\z)))
      (and (>= (char-code char) (char-code #\A))
           (<= (char-code char) (char-code #\Z)))
      (and (>= (char-code char) (char-code #\0))
           (<= (char-code char) (char-code #\9)))
      (eq char #\_)
      (eq char #\-)
      (eq char #\~)
      (eq char #\!)
      (eq char #\\)
      (and (eq char #\:) (eq pos 1))
      (eq char #\/)))


(defun check_path_char (name x y)
  (or (> x y)
      (and (ok_pathchar (schar name x) x)
           (check_path_char name (+ x 1) y))))

(defun ok_pathname (name)
  (if (and (check_path_char name 0 (- (length name) 1))
           (not (search "\\\\" name)))
      T
    nil))



(defun mkdir (path)
  (select platform
          ('WATCOM  (make_dir path))
          ('UNIX
           (print (remove-slash path))
           (make_dir path))))



  (do ((ok nil nil))
      ((eq ok T) nil)

      (let ((install-path  (nice_input title path-prompt
                                       (select platform
                                               ('WATCOM "c:\\abuse")
                                               ('UNIX  "~/abuse")))))
        (if (not install-path) (quit-install)
          (if (not (ok_pathname install-path))
              (if (not (show_yes_no title path_not_valid yes-key no-key))
                       (quit))
            (let ((install-path (modify_install_path (append-slash install-path))))
              (if (or (dir_exsist (remove-slash install-path))
                      (and (show_yes_no title (cons install-path make-dir?) yes-key no-key)
                           (if (mkdir install-path)
                               T
                             (if (show_yes_no title (list install-path mkdir-failed) yes-key no-key)
                                 nil
                               (quit)))))
                  (if (install install-path)
                      (progn
                        (go_there install-path)
                        (setq ok T)))))))))


(select lang
        ('french (progn 
		   (open_file "lisp/english.lsp" "wb"  (print `(load ,(concatenate 'string '(#\") "lisp/french.lsp" '(#\")  ))))
		   (system "del setup.exe")
		   (system "del setup.ini")
		   (system "rename fren_set.exe setup.exe")
		   (system "rename fsetup.ini setup.ini")
		   (system "del germ_set.exe")
		   (system "del gsetup.ini")
		   ))
        ('german (progn
		   (open_file "lisp/english.lsp" "wb"  (print `(load ,(concatenate 'string '(#\") "lisp/german.lsp" '(#\")  ))))
		   (system "del setup.exe")
		   (system "del setup.ini")
		   (system "rename germ_set.exe setup.exe")
		   (system "rename gsetup.ini setup.ini")
		   (system "del fren_set.exe")
		   (system "del fsetup.ini")
		   ))
	('english (progn
		    (system "del gsetup.ini")
		    (system "del fsetup.ini")
		    (system "del fren_set.exe")
		    (system "del germ_set.exe"))))
		 


(print start-msg)





