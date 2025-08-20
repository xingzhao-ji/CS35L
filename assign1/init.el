(defvar persist-drib-target-file nil
  "The file where dribble contents will be appended.")

(defvar persist-drib-temp-file nil
  "The temporary dribble file path.")

(defun persist-drib-hook-function ()
  "Function added to kill-emacs-hook to append dribble file contents."
  (when (and persist-drib-temp-file
             persist-drib-target-file
             (file-exists-p persist-drib-temp-file)
             (file-readable-p persist-drib-temp-file))
    (with-temp-buffer
      (insert-file-contents persist-drib-temp-file)
      (when (> (buffer-size) 0)
        (append-to-file (point-min) (point-max) persist-drib-target-file)
        (message "Dribble contents appended to %s" persist-drib-target-file)))))

(defun persist-drib (filename)
  "Start a dribble file with timestamp and append its contents to FILENAME on Emacs exit.
   The dribble file will record all keystrokes until Emacs exits."
  (interactive "FFile to append dribble contents to: ")
  (let ((timestamp (format-time-string "%Y%m%d-%H%M%S")))
    ;; Set our global variables
    (setq persist-drib-target-file (expand-file-name filename))
    (setq persist-drib-temp-file (expand-file-name 
                                  (format "dribble-%s.txt" timestamp)
                                  temporary-file-directory))
    
    ;; Start the dribble file
    (open-dribble-file persist-drib-temp-file)
    
    ;; Add the hook if not already there
    (add-hook 'kill-emacs-hook 'persist-drib-hook-function)))
