(*
 * Test Suite Exhaustivo para COOL Lexer
 * Cubre todos los tokens válidos, anidamiento de comentarios y escapes en strings
 *)

-- Comentario de una sola línea inicial

class Main inherits IO {
    -- Atributos de prueba
    x : Int <- 42;
    flag_true : Bool <- true;
    flag_false : Bool <- false;
    flag_mixed : Bool <- tRuE;
    not_bool : Bool <- fAlSe;
    type_bool_1 : TypeID <- True;   -- Debe ser TYPEID, no BOOL_CONST
    type_bool_2 : TypeID <- False;  -- Debe ser TYPEID, no BOOL_CONST
    obj_self : Object <- self;      -- Debe ser OBJECTID
    type_self : SELF_TYPE <- self;  -- Debe ser TYPEID

    -- Comentarios de bloque anidados
    (* Primer nivel
       (* Segundo nivel
          (* Tercer nivel con operadores: + - * / <= <- => *)
          Regreso al segundo nivel
       *)
       Regreso al primer nivel
       -- comentario de una línea dentro de bloque
       "string dentro de comentario (* *)"
    *)

    -- Cadenas con secuencias de escape
    str_normal : String <- "Hola Mundo";
    str_escapes : String <- "Escape: \b (backspace), \t (tab), \n (newline), \f (formfeed)";
    str_quote_bs : String <- "Comilla: \" y Backslash: \\";
    str_digit_zero : String <- "Secuencia backslash cero: \0 debe dar '0'";
    str_multiline : String <- "Cadena dividida \
en múltiples líneas físicas \
con salto escapado";
    str_with_symbols : String <- "Texto con símbolos: -- (* *) <= <- =>";

    -- Método con todas las palabras clave y operadores
    test_method(a : Int, b : Int) : Object {
        let y : Int <- a + b * ~2 / (x - 1) in
        if y <= 10 then
            while not (y = 0) loop
                {
                    y <- y - 1;
                    out_int(y);
                }
            pool
        else
            case isvoid self of
                o : Object => out_string("Es objeto\n");
                m : Main   => new Main;
            esac
        fi
    };

    main() : Object {
        test_method(10, 20)
    };
};
