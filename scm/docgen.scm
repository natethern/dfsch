#!/usr/bin/env dfsch-repl
; documentation generator for dfsch

(require 'introspect)
(require 'sxml)
(require 'inet)
(require 'cmdopts)
(require 'unix)

(define (directory? path)
  (let ((stat (unix:stat path)))
    (if (null? stat)
        ()
        (stat 'isdir))))

(define (ensure-directory path)
  (unless (directory? path)
          (unix:mkdir path 0755)))

(define *clean-toplevel* (make-default-environment))

(define (get-toplevel-variables)
  (get-variables *clean-toplevel*))

(define (get-object-documentation object)
  (cond
   ((instance? object <function>)
    (slot-ref object 'documentation))
   ((instance? object <form>)
    (slot-ref object 'documentation))
   ((instance? object <macro>)
    (get-object-documentation (slot-ref object 'procedure)))
   (else ())))

(define (make-counter)
  (let ((count 0))
    (lambda ()
      (set! count (+ 1 count))
      count)))

(define (variables->name+doc lyst)
  (let ((id-counter (make-counter)))
    (sort-alist
     (map 
      (lambda (x) 
        (let ((name (car x))
              (value (cadr x)))
          (list (symbol->string name) 
                (type-name (type-of value))
                (format "it~x" (id-counter))
                (get-object-documentation value)
                value)))
      lyst))))
  
(define (sort-alist alist)
  (sort-list! alist
             (lambda (x y)
               (string<? (car x)
                         (car y)))))

(define (target-name str) 
  (inet:uri-base64-encode str))

(define (make-index-list list link-to target)
  `((ul ,@(map (lambda (item)
                 (let ((name (car item))
                       (type-name (cadr item))
                       (id (caddr item)))
                   `(li (a (@ (href ,(string-append link-to "#" id))
                              ,@(unless (null? target)
                                        `((target ,target))))
                           ,name)
                        " " ,type-name)))
               list))))

(define (make-documentation-body lyst)
  (map (lambda (item)
         (let ((name (car item))
               (type (cadr item))
               (id (caddr item))
               (documentation (cadddr item)))
           `(div (@ (id ,id) (title ,name))
                 (h2 ,(string-append type " " name))
                 (pre ,(format "~w" (list-ref item 4)))
                 (p ,documentation))))
       lyst))

(define (html-frameset title)
  `(html 
    (@ (xmlns "http://www.w3.org/1999/xhtml"))
    (head ,@(when (not (null? title))
                  `((title ,title))))
    (frameset (@ (cols "250,*"))
     (frame (@ (name "index") (src "list.html")))
     (frame (@ (name "body") (src "body.html"))))))

(define (html-boiler-plate title infoset)
  `(html 
    (@ (xmlns "http://www.w3.org/1999/xhtml"))
    (head ,@(when (not (null? title))
                  `((title ,title))))
    (body ,@(when (not (null? title))
                  `((h1 ,title)))
          ,@infoset
          (p "Generated by docgen.scm running on dfsch " ,*dfsch-version*))))


(define (emit-documentation lyst directory title)
  (ensure-directory directory)
  (sxml:emit-file (html-frameset title) 
                  (string-append directory "/index.html"))
  (sxml:emit-file (html-boiler-plate 
                   ()
                   (make-index-list lyst "body.html" "body"))
                  (string-append directory "/list.html"))
  (sxml:emit-file (html-boiler-plate 
                   title
                   (make-documentation-body lyst))
                  (string-append directory "/body.html")))
  

(define (emit-core-documentation directory)
  (emit-documentation 
   (variables->name+doc (get-toplevel-variables))
   directory
   "Default dfsch top-level environment"))

(when (defined? *posix-argv*)
      (emit-core-documentation (cadr *posix-argv*)))

