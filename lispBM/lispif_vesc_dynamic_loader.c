/*
	Copyright 2022 Benjamin Vedder	benjamin@vedder.se

	This file is part of the VESC firmware.

	The VESC firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The VESC firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lispif.h"
#include "lispbm.h"

static const char* functions[] = {
"(defun uart-read-bytes (buffer n ofs)"
"(let ((rd (uart-read buffer n ofs)))"
"(if (= rd n)"
"(bufset-u8 buffer (+ ofs rd) 0)"
"(progn (yield 4000) (uart-read-bytes buffer (- n rd) (+ ofs rd)))"
")))",

"(defun uart-read-until (buffer n ofs end)"
"(let ((rd (uart-read buffer n ofs end)))"
"(if (and (> rd 0) (or (= rd n) (= (bufget-u8 buffer (+ ofs (- rd 1))) end)))"
"(bufset-u8 buffer (+ ofs rd) 0)"
"(progn (yield 10000) (uart-read-until buffer (- n rd) (+ ofs rd) end))"
")))",

"(defun map (f lst)"
"(if (eq lst nil) nil "
"(cons (f (car lst)) (map f (cdr lst)))))",

"(defun iota (n)"
"(let ((iacc (lambda (acc i)"
"(if (< i 0) acc (iacc (cons i acc) (- i 1))))))"
"(iacc nil (- n 1))))",

"(defun range (start end)"
"(map (lambda (x) (+ x start)) (iota (- end start))))",

"(defun foldl (f init lst)"
"(if (eq lst nil) init (foldl f (f init (car lst)) (cdr lst))))",

"(defun foldr (f init lst)"
"(if (eq lst nil) init (f (car lst) (foldr f init (cdr lst)))))",

"(defun reverse (lst)"
"(let ((revacc (lambda (acc lst)"
"(if (eq nil lst) acc (revacc (cons (car lst) acc) (cdr lst))))))"
"(revacc nil lst)))",

"(defun length (lst)"
"(let ((len (lambda (l lst)"
"(if (eq lst nil) l (len (+ l 1) (cdr lst))))))"
"(len 0 lst)))",

"(defun apply (f lst) (eval `(,f ,@lst)))",

"(defun zipwith (f x y)"
"(let ((map-rec (lambda (f res lst ys)"
"(if (eq lst nil)"
"(reverse res)"
"(map-rec f (cons (f (car lst) (car ys)) res) (cdr lst) (cdr ys))))))"
"(map-rec f nil x y)))",

"(defun sleep (seconds) (yield (* seconds 1000000.0)))",

"(defun filter (f lst)"
"(let ((filter-rec (lambda (f lst ys)"
"(if (eq lst nil)"
"(reverse ys)"
"(if (f (car lst))"
"(filter-rec f (cdr lst) (cons (car lst) ys))"
"(filter-rec f (cdr lst) ys))))))"
"(filter-rec f lst nil)"
"))",

"(defun str-len (str) (- (buflen str) 1))",

"(defun sort (f lst)"
"(let ((insert (lambda (elt f sorted-lst)"
"(if (eq sorted-lst nil) (list elt)"
"(if (f elt (car sorted-lst)) (cons elt sorted-lst)"
"(cons (car sorted-lst) (insert elt f (cdr sorted-lst))))))))"
"(if (eq lst nil) nil (insert (car lst) f (sort f (cdr lst))))))",

"(defun str-cmp-asc (a b) (< (str-cmp a b) 0))",
"(defun str-cmp-dsc (a b) (> (str-cmp a b) 0))",

"(defun second (x) (car (cdr x)))",
"(defun third (x) (car (cdr (cdr x))))",
};

static const char* macros[] = {
"(define defun (macro (name args body) `(define ,name (lambda ,args ,body))))",

"(define loopfor (macro (it start cond update body)"
"`(let ((loop (lambda (,it res break)(if ,cond (loop ,update ,body break) res"
"))))(call-cc (lambda (brk) (loop ,start nil brk))))))",

"(define loopwhile (macro (cond body)"
"`(let ((loop (lambda (res break)(if ,cond (loop ,body break) res"
"))))(call-cc (lambda (brk) (loop nil brk))))))",

"(define looprange (macro (it start end body)"
"`(let ((loop (lambda (,it res break)(if (< ,it ,end)(loop (+ ,it 1),body break) res"
"))))(call-cc (lambda (brk) (loop ,start nil brk))))))",

"(define loopforeach (macro (it lst body)"
"`(let ((loop (lambda (,it rst res break)"
"(if (eq ,it nil) res (loop (car rst) (cdr rst) ,body break)"
"))))(call-cc (lambda (brk) (loop (car ,lst) (cdr ,lst) nil brk))))))",
};

static bool strmatch(const char *str1, const char *str2) {
	unsigned int len = strlen(str1);

	if (str2[len] != ' ') {
		return false;
	}

	bool same = true;
	for (unsigned int i = 0;i < len;i++) {
		if (str1[i] != str2[i]) {
			same = false;
			break;
		}
	}

	return same;
}

bool lispif_vesc_dynamic_loader(const char *str, const char **code) {
	for (unsigned int i = 0; i < (sizeof(macros) / sizeof(macros[0]));i++) {
		if (strmatch(str, macros[i] + 8)) {
			*code = macros[i];
			return true;
		}
	}

	for (unsigned int i = 0; i < (sizeof(functions) / sizeof(functions[0]));i++) {
		if (strmatch(str, functions[i] + 7)) {
			*code = functions[i];
			return true;
		}
	}

	return false;
}
