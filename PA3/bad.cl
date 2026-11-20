(*
 * bad.cl - Programa COOL con errores semánticos estáticos intencionales
 * Diseñado para verificar que el analizador semántico estático detecta y reporta
 * adecuadamente las violaciones del Manual de COOL (CS143) agrupadas por categorías.
 *
 * Las categorías corresponden a la Sección 7 del enunciado del Laboratorio 03:
 * - A: Clases y grafo de herencia
 * - B: Atributos
 * - C: Métodos
 * - D: Identificadores y alcance
 * - E: Expresiones
 *)

-- ========================================================
-- CATEGORÍA A: Clases y Grafo de Herencia
-- ========================================================

-- Error A.2: Redefinición de una clase básica (Int)
class Int {
    x : Int;
};

-- Error A.3: Una clase llamada SELF_TYPE
class SELF_TYPE {
    val : Int;
};

-- Error A.4: Herencia de una clase no definida (Inexistente)
class SubInexistente inherits Inexistente {
    x : Int;
};

-- Error A.5: Herencia de clases selladas básicas (Bool, String)
class SubBool inherits Bool {
    b : Bool;
};

class SubString inherits String {
    s : String;
};

-- Error A.1: Clase definida más de una vez
class DuplicateClass {
    x : Int;
};

class DuplicateClass {
    y : Int;
};

-- Error A.6: Ciclo en el grafo de herencia (CycleA -> CycleB -> CycleA)
class CycleA inherits CycleB {
    a : Int;
};

class CycleB inherits CycleA {
    b : Int;
};

-- ========================================================
-- CATEGORÍA B: Atributos
-- ========================================================

class BaseAttr {
    inherited_attr : Int;
};

class SubAttr inherits BaseAttr {
    -- Error B.1: Atributo llamado self
    self : Int;

    -- Error B.2: Atributo definido más de una vez en la misma clase
    dup_attr : Int;
    dup_attr : String;

    -- Error B.3: Atributo heredado redefinido en una subclase
    inherited_attr : Int;

    -- Error B.4: Tipo de atributo que no corresponde a una clase definida
    bad_type_attr : TipoDesconocido;

    -- Error B.5: Inicialización de atributo cuyo tipo no conforma con el declarado
    mismatch_init : Int <- "Texto no es entero";
};

-- ========================================================
-- CATEGORÍA C: Métodos
-- ========================================================

class BaseMethod {
    foo(x : Int, y : Bool) : Int {
        x
    };

    bar() : Object {
        new Object
    };
};

class SubMethod inherits BaseMethod {
    -- Error C.1: Método definido más de una vez en la misma clase
    unique_meth() : Int { 1 };
    unique_meth() : Int { 2 };

    -- Error C.2: Parámetros formales con nombres repetidos
    dup_params(p : Int, p : String) : Int { 0 };

    -- Error C.3: Parámetro formal llamado self
    self_param(self : Int) : Int { 0 };

    -- Error C.4: Parámetro formal de tipo SELF_TYPE
    selftype_param(x : SELF_TYPE) : Int { 0 };

    -- Error C.5: Tipo de parámetro o de retorno no definido
    undef_param_ret(x : TipoRaro) : RetornoRaro { 0 };

    -- Error C.6: Cuerpo de método cuyo tipo no conforma con el retorno declarado
    mismatch_body() : Int {
        "Esto no es un Int"
    };

    -- Error C.6 (caso especial SELF_TYPE): Retorno declarado SELF_TYPE pero cuerpo de tipo concreto
    selftype_ret_mismatch() : SELF_TYPE {
        new BaseMethod
    };

    -- Error C.7: Override con número incompatible de parámetros (esperaba 2, recibe 1)
    foo(x : Int) : Int {
        x
    };

    -- Error C.7: Override con tipo de retorno diferente (esperaba Object, declara Int)
    bar() : Int {
        10
    };
};

-- ========================================================
-- CATEGORÍA D: Identificadores y Alcance
-- CATEGORÍA E: Expresiones
-- ========================================================

class TestExpressions {
    x : Int;
    b : Bool;
    s : String;

    test_scope_and_expr() : Object {
        {
            -- Error D.1: Uso de identificador no declarado
            variable_fantasma <- 42;

            -- Error D.2: Asignación a self
            self <- new TestExpressions;

            -- Error E.1: Asignación cuyo tipo no conforma
            x <- "cadena";

            -- Error D.3 y E.2: self usado como variable en let, tipo de let no conforme
            let self : Int <- 10 in 5;
            let y : Int <- "no es int" in y + 1;

            -- Error D.4: Tipo de let no definido
            let z : TipoInexistente in 0;

            -- Error E.5: Condición de if que no es Bool
            if 10 + 20 then
                1
            else
                2
            fi;

            -- Error E.5: Condición de while que no es Bool
            while "no booleano" loop
                x <- x + 1
            pool;

            -- Error E.6: Operandos de aritmética y comparación no enteros
            x <- 10 + "cinco";
            b <- true < 5;

            -- Error E.7: Operando de negación no entero / not no booleano
            x <- ~"cadena";
            b <- not 42;

            -- Error E.8: Comparación de igualdad ilegal entre tipo básico y otro tipo
            b <- 5 = "cinco";
            b <- true = 1;

            -- Error D.4 y E.4: new con tipo indefinido / static dispatch con tipo no conforme o indefinido
            let obj : Object <- new ClaseInexistente in 0;
            let base : BaseMethod <- new BaseMethod in base@Int.foo(1, true);

            -- Error E.3: Dispatch a método indefinido / número o tipos incorrectos
            let bm : BaseMethod <- new BaseMethod in {
                bm.metodo_inexistente();
                bm.foo(1);           -- faltan argumentos
                bm.foo(1, "no bool"); -- argumento 2 no conforma
            };

            -- Error D.3, D.4 y E.9: Case con self, tipo duplicado o tipo indefinido
            case x of
                self : Int => 1;
                c1 : Int => 2;
                c2 : Int => 3;
                c3 : TipoDesconocidoCase => 4;
                c4 : SELF_TYPE => 5;
            esac;
        }
    };
};

-- ========================================================
-- CLASE MAIN REQUERIDA (con error C para probar reporte completo)
-- ========================================================

-- Nota: Si se elimina Main se produce error A.7.
-- Aquí definimos Main pero con un método main que toma parámetros (Error A.8).
class Main {
    main(arg : Int) : Object {
        arg
    };
};
