(define-derived-mode kale-mode lisp-data-mode "kale"
  "Kale editing mode."
  (enable-paredit-mode))

(add-to-list 'auto-mode-alist '("\\.kale\\'" . kale-mode))
