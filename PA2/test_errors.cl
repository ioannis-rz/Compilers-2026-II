-- Casos de prueba para errores léxicos en COOL

-- 1. Cierre de comentario sin apertura previa (Unmatched *))
*)

-- 2. Caracteres no válidos (no reconocidos por COOL)
#
$
!
^
?

-- 3. String con salto de línea no escapado (Unterminated string constant)
"Esta cadena no cierra
sigue aquí"

-- 4. String con carácter nulo
-- "hola\0mundo" (manejado en archivos de grading con byte 0x00)

-- 5. Comentario que llega a fin de archivo sin cerrarse (EOF in comment)
(* Este comentario nunca se cerrará...
