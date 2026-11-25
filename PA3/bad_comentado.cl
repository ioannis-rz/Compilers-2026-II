(*
 * ==============================================================================
 * ARCHIVO: bad_comentado.cl
 * PROYECTO: Laboratorio 03 - Analizador Semántico para COOL (PA4)
 * ASIGNATURA: Compiladores, Universidad Nacional de Colombia (2026-II)
 * ==============================================================================
 * PROPÓSITO:
 * Programa de prueba negativo que contiene intencionalmente errores semánticos
 * estáticos agrupados por las categorías de la Sección 7 del enunciado del Lab03.
 *
 * Cada error está documentado con:
 * 1. El código de error (A.1 a E.9).
 * 2. La regla semántica del manual de COOL que se viola.
 * 3. El mensaje esperado que emite semant.cc.
 * ==============================================================================
 *)

-- =============================================================================
-- CATEGORÍA A: CLASES Y GRAFO DE HERENCIA
-- =============================================================================

-- ERROR A.2: Redefinición de una clase básica (Int).
-- Regla: Las 5 clases básicas (Object, IO, Int, Bool, String) no pueden redefinirse.
-- Mensaje esperado: "Redefinition of basic class Int."
class Int {
    x : Int;
};

-- ERROR A.3: Una clase con nombre reservado SELF_TYPE.
-- Regla: SELF_TYPE es una palabra clave reservada del sistema de tipos; no es una clase.
-- Mensaje esperado: "Redefinition of basic class SELF_TYPE."
class SELF_TYPE {
    val : Int;
};

-- ERROR A.4: Herencia de una clase no definida en el programa.
-- Regla: La clase padre indicada en 'inherits P' debe existir en name_to_class.
-- Mensaje esperado: "Class SubInexistente inherits from an undefined class Inexistente."
class SubInexistente inherits Inexistente {
    x : Int;
};

-- ERROR A.5: Herencia de clases selladas (Bool, String, Int).
-- Regla: Int, String y Bool son clases finales/selladas; no se permite subclasificarlas.
-- Mensajes esperados:
-- "Class SubBool cannot inherit class Bool."
-- "Class SubString cannot inherit class String."
class SubBool inherits Bool {
    b : Bool;
};

class SubString inherits String {
    s : String;
};

-- ERROR A.1: Clase definida más de una vez en el mismo programa.
-- Regla: No puede haber dos declaraciones con el mismo nombre de clase.
-- Mensaje esperado: "Class DuplicateClass was previously defined."
class DuplicateClass {
    x : Int;
};

class DuplicateClass {
    y : Int;
};

-- ERROR A.6: Ciclo en el grafo de herencia (CycleA -> CycleB -> CycleA).
-- Regla: El grafo de herencia debe ser un árbol con raíz única en Object; los ciclos son ilegales.
-- Mensaje esperado: "Class CycleA, or an ancestor of CycleA, is involved in an inheritance cycle."
class CycleA inherits CycleB {
    a : Int;
};

class CycleB inherits CycleA {
    b : Int;
};

-- =============================================================================
-- CATEGORÍA B: ATRIBUTOS
-- =============================================================================

class BaseAttr {
    inherited_attr : Int;
};

class SubAttr inherits BaseAttr {
    -- ERROR B.1: Atributo llamado 'self'.
    -- Regla: 'self' es un identificador reservado que liga la instancia actual;
    -- está estrictamente prohibido usarlo como nombre de atributo.
    -- Mensaje esperado: "'self' cannot be the name of an attribute."
    self : Int;

    -- ERROR B.2: Atributo definido más de una vez en la misma clase.
    -- Regla: Los nombres de atributos deben ser únicos dentro de cada clase.
    -- Mensaje esperado: "Attribute dup_attr is multiply defined."
    dup_attr : Int;
    dup_attr : String;

    -- ERROR B.3: Atributo heredado redefinido en una subclase.
    -- Regla: En COOL, a diferencia de C++ o Java, los atributos NO pueden sobreescribirse.
    -- Mensaje esperado: "Attribute inherited_attr is an attribute of an inherited class."
    inherited_attr : Int;

    -- ERROR B.4: Tipo de atributo que no corresponde a una clase definida.
    -- Regla: El tipo declarado en 'a : T' debe ser una clase existente o SELF_TYPE.
    -- Mensaje esperado: "Type TipoDesconocido of attribute bad_type_attr is undefined."
    bad_type_attr : TipoDesconocido;

    -- ERROR B.5: Inicialización de atributo cuyo tipo no conforma con el declarado.
    -- Regla: Si 'a : T <- e', se debe cumplir tipo(e) <= T. Aquí String no conforma con Int.
    -- Mensaje esperado: "Inferred type String of initialization of attribute mismatch_init does not conform to declared type Int."
    mismatch_init : Int <- "Texto no es entero";
};

-- =============================================================================
-- CATEGORÍA C: MÉTODOS
-- =============================================================================

class BaseMethod {
    foo(x : Int, y : Bool) : Int {
        x
    };

    bar() : Object {
        new Object
    };
};

class SubMethod inherits BaseMethod {
    -- ERROR C.1: Método definido más de una vez en la misma clase.
    -- Regla: Cada clase solo puede tener una definición por cada nombre de método.
    -- Mensaje esperado: "Method unique_meth is multiply defined."
    unique_meth() : Int { 1 };
    unique_meth() : Int { 2 };

    -- ERROR C.2: Parámetros formales con nombres repetidos.
    -- Regla: Todos los parámetros formales de un método deben tener nombres distintos.
    -- Mensaje esperado: "Formal parameter p is multiply defined."
    dup_params(p : Int, p : String) : Int { 0 };

    -- ERROR C.3: Parámetro formal llamado 'self'.
    -- Regla: 'self' no puede usarse como nombre de parámetro formal.
    -- Mensaje esperado: "'self' cannot be the name of a formal parameter."
    self_param(self : Int) : Int { 0 };

    -- ERROR C.4: Parámetro formal con tipo declared SELF_TYPE.
    -- Regla: SELF_TYPE solo es válido como tipo de retorno, atributo, let o new;
    -- está explícitamente prohibido como tipo de parámetro formal.
    -- Mensaje esperado: "Formal parameter x cannot have type SELF_TYPE."
    selftype_param(x : SELF_TYPE) : Int { 0 };

    -- ERROR C.5: Tipo de parámetro o de retorno no definido en el programa.
    -- Regla: Todo tipo especificado en la firma debe ser una clase válida o SELF_TYPE.
    -- Mensajes esperados:
    -- "Type TipoRaro of parameter x is undefined."
    -- "Undefined return type RetornoRaro in method undef_param_ret."
    undef_param_ret(x : TipoRaro) : RetornoRaro { 0 };

    -- ERROR C.6: Cuerpo de método cuyo tipo no conforma con el retorno declarado.
    -- Regla [Method]: Si el método declara retornar T, tipo(cuerpo) <= T.
    -- Aquí declara Int pero el cuerpo es String ("Esto no es un Int").
    -- Mensaje esperado: "Inferred return type String of method mismatch_body does not conform to declared return type Int."
    mismatch_body() : Int {
        "Esto no es un Int"
    };

    -- ERROR C.6 (variante SELF_TYPE): Retorno declarado SELF_TYPE pero cuerpo de tipo concreto.
    -- Regla especial: Cuando el retorno es SELF_TYPE, el cuerpo DEBE evaluarse a SELF_TYPE.
    -- Un tipo concreto como BaseMethod no es suficiente aunque sea ancestro.
    -- Mensaje esperado: "Inferred return type BaseMethod of method selftype_ret_mismatch does not conform to declared return type SELF_TYPE."
    selftype_ret_mismatch() : SELF_TYPE {
        new BaseMethod
    };

    -- ERROR C.7: Sobreescritura (override) con número incompatible de parámetros.
    -- Regla: En COOL la firma debe ser estrictamente idéntica. BaseMethod::foo recibe 2, aquí se pasa 1.
    -- Mensaje esperado: "Incompatible number of formal parameters in redefined method foo."
    foo(x : Int) : Int {
        x
    };

    -- ERROR C.7: Sobreescritura con tipo de retorno diferente.
    -- Regla: BaseMethod::bar() declara Object; SubMethod::bar() declara Int.
    -- COOL exige invariancia estricta de firmas (no permite covarianza).
    -- Mensaje esperado: "In redefined method bar, return type Int is different from original type Object."
    bar() : Int {
        10
    };
};

-- =============================================================================
-- CATEGORÍAS D y E: IDENTIFICADORES, ALCANCE Y EXPRESIONES
-- =============================================================================

class TestExpressions {
    x : Int;
    b : Bool;
    s : String;

    test_scope_and_expr() : Object {
        {
            -- ERROR D.1: Uso de identificador no declarado en el ambiente O.
            -- Mensaje esperado: "Assignment to undeclared variable variable_fantasma."
            variable_fantasma <- 42;

            -- ERROR D.2: Intento de asignar a la variable especial 'self'.
            -- Regla: 'self' es de solo lectura (read-only).
            -- Mensaje esperado: "Cannot assign to 'self'."
            self <- new TestExpressions;

            -- ERROR E.1: Asignación cuyo tipo no conforma con el tipo declarado.
            -- Regla: x es Int; la expresión asignada es String. String no conforma con Int.
            -- Mensaje esperado: "Type String of assigned expression does not conform to declared type Int of identifier x."
            x <- "cadena";

            -- ERROR D.3: 'self' usado como identificador de variable en una expresión 'let'.
            -- Mensaje esperado: "'self' cannot be bound in a 'let' expression."
            let self : Int <- 10 in 5;

            -- ERROR E.2: Inicialización de variable let cuyo tipo no conforma.
            -- Mensaje esperado: "Inferred type String of initialization of y does not conform to identifier's declared type Int."
            let y : Int <- "no es int" in y + 1;

            -- ERROR D.4: Tipo de variable let no definido.
            -- Mensaje esperado: "Type TipoInexistente of let-bound identifier z is undefined."
            let z : TipoInexistente in 0;

            -- ERROR E.5: Condición del condicional 'if' que no es de tipo Bool.
            -- Regla: El predicado de 'if' debe evaluarse obligatoriamente a Bool (aquí es Int: 10 + 20).
            -- Mensaje esperado: "Predicate of 'if' does not have type Bool."
            if 10 + 20 then
                1
            else
                2
            fi;

            -- ERROR E.5: Condición del bucle 'while' que no es de tipo Bool.
            -- Regla: El predicado de 'while' debe ser de tipo Bool (aquí es String: "no booleano").
            -- Mensaje esperado: "Loop condition does not have type Bool."
            while "no booleano" loop
                x <- x + 1
            pool;

            -- ERROR E.6: Operandos de aritmética con tipos no enteros.
            -- Regla: '+' exige que ambos argumentos sean estrictamente Int.
            -- Mensaje esperado: "non-Int arguments: Int + String"
            x <- 10 + "cinco";

            -- ERROR E.6: Operando de comparación con tipos incompatibles.
            -- Regla: '<' exige que ambos operandos sean Int.
            -- Mensaje esperado: "non-Int arguments: Bool < Int"
            b <- true < 5;

            -- ERROR E.7: Negación entera (~) aplicada a un tipo no entero (String).
            -- Mensaje esperado: "Argument of '~' has type String instead of Int."
            x <- ~"cadena";

            -- ERROR E.7: Negación booleana (not) aplicada a un tipo no booleano (Int).
            -- Mensaje esperado: "Argument of 'not' has type Int instead of Bool."
            b <- not 42;

            -- ERROR E.8: Comparación de igualdad (=) ilegal entre un tipo básico y otro tipo.
            -- Regla: Si cualquiera de los dos lados es Int, Bool o String, el otro lado
            -- debe ser exactamente del MISMO tipo básico.
            -- Mensaje esperado: "Illegal comparison with a basic type."
            b <- 5 = "cinco";
            b <- true = 1;

            -- ERROR D.4 y E.4: 'new' con clase indefinida.
            -- Mensaje esperado: "'new' used with undefined class ClaseInexistente."
            let obj : Object <- new ClaseInexistente in 0;

            -- ERROR E.4: Despacho estático (@) con tipo que no conforma.
            -- Regla: En e@T.m(), tipo(e) debe conformar con T. BaseMethod no conforma con Int.
            -- Mensaje esperado: "Expression type BaseMethod does not conform to declared static dispatch type Int."
            let base : BaseMethod <- new BaseMethod in base@Int.foo(1, true);

            -- ERROR E.3: Despacho a método inexistente o con argumentos incorrectos:
            let bm : BaseMethod <- new BaseMethod in {
                -- Mensaje: "Dispatch to undefined method metodo_inexistente."
                bm.metodo_inexistente();

                -- Mensaje: "Method foo called with wrong number of arguments."
                bm.foo(1);

                -- Mensaje: "In call of method foo, type String of parameter y does not conform to declared type Bool."
                bm.foo(1, "no bool");
            };

            -- ERROR D.3, D.4 y E.9: Errores en sentencia case:
            -- 1. 'self' como variable de rama: "'self' bound in 'case'."
            -- 2. Ramas duplicadas con el mismo tipo: "Duplicate branch Int in case statement."
            -- 3. Tipo de rama no definido: "Class TipoDesconocidoCase of case branch is undefined."
            -- 4. Tipo de rama SELF_TYPE: "Identifier c4 declared with type SELF_TYPE in case branch."
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

-- =============================================================================
-- ERROR A.7 / A.8: CLASE MAIN INVÁLIDA (CON PARÁMETROS)
-- =============================================================================
-- Regla: Si no existiera Main, emitiría: "Class Main is not defined." (Error A.7).
-- Al definir Main con un método 'main(arg : Int)', emitimos el Error A.8:
-- Mensaje esperado: "'main' method in class Main should have no arguments."
class Main {
    main(arg : Int) : Object {
        arg
    };
};
