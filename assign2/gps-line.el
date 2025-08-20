(defun gps-line ()
  (interactive)
  (let* ((start (point-min))
         (narrowed-line (line-number-at-pos))
         (full-line (if (= start 1)
                        narrowed-line
                      (+ narrowed-line (line-number-at-pos start) -1)))
         (total-lines
          (save-excursion
            (save-restriction
              (widen)
              (goto-char (point-min))
              (count-matches "\n" nil (point-max))))))
    (if (= start 1)
        (message "Line %d/%d" full-line total-lines)
      (message "Line %d (narrowed %d)/%d" full-line narrowed-line total-lines))))
