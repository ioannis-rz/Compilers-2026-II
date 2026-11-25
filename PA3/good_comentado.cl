(*
 * ==============================================================================
 * ARCHIVO: good_comentado.cl
 * PROYECTO: Laboratorio 03 - Analizador Semántico para COOL (PA4)
 * ASIGNATURA: Compiladores, Universidad Nacional de Colombia (2026-II)
 * ==============================================================================
 * PROPÓSITO:
 * Programa COOL sintáctica y semánticamente correcto. Demuestra la validez
 * de todas las reglas del sistema de tipos estáticos de COOL especificadas
 * en el Manual de COOL (CS143) y el enunciado del Laboratorio 03.
 *
 * El analizador semántico debe procesar este archivo sin emitir ningún error
 * y anotando correctamente el tipo de cada nodo del AST.
 * ==============================================================================
 *)

-- -----------------------------------------------------------------------------
-- 1. CLASE BASE CON HERENCIA DE UNA CLASE INTEGRADA (IO)
-- -----------------------------------------------------------------------------
-- Animal hereda de IO para tener acceso a los métodos integrados:
-- out_string(), out_int(), in_string(), in_int().
class Animal inherits IO {
    -- Declaración de atributos sin inicialización explícita:
    -- name se inicializará implícitamente a void (cadena vacía en runtime de COOL).
    name : String;
    -- age se inicializará implícitamente a 0 en el runtime de COOL.
    age  : Int;

    -- LUGAR 1 DE SELF_TYPE: Como tipo de retorno de un método.
    -- Regla [Method]: Cuando el retorno declarado es SELF_TYPE, el cuerpo del método
    -- DEBE evaluarse estáticamente a SELF_TYPE (en este caso, la última expresión es 'self').
    init(n : String, a : Int) : SELF_TYPE {
        {
            -- Asignación válida: 'n' es de tipo String, 'name' es String (String <= String).
            name <- n;
            -- Asignación válida: 'a' es de tipo Int, 'age' es Int (Int <= Int).
            age <- a;
            -- 'self' siempre tiene el tipo estático SELF_TYPE dentro de la clase actual.
            self;
        }
    };

    -- Métodos accesores (getters) estándar con retornos primitivos:
    get_name() : String {
        name
    };

    get_age() : Int {
        age
    };

    -- Método que será sobreescrito (overridden) en subclases:
    speak() : String {
        "Sonido generico de animal"
    };

    -- LUGAR 2 DE SELF_TYPE: Expresión 'new SELF_TYPE'.
    -- Regla [New]: 'new SELF_TYPE' crea una nueva instancia de la clase dinámica
    -- exacta del objeto receptor en tiempo de ejecución, preservando el polimorfismo.
    clone_self() : SELF_TYPE {
        new SELF_TYPE
    };
};

-- -----------------------------------------------------------------------------
-- 2. JERARQUÍA INTERMEDIA: Mammal hereda de Animal
-- -----------------------------------------------------------------------------
class Mammal inherits Animal {
    -- Inicialización explícita de atributo:
    -- 'true' es Bool, 'has_fur' es Bool. Conforma Bool <= Bool.
    has_fur : Bool <- true;

    -- SOBREESCRITURA VÁLIDA (Override):
    -- Regla [Method-Override]: Mismo nombre ("speak"), mismo número de parámetros (0)
    -- y mismo tipo de retorno ("String"). Cumple la invariancia de firmas de COOL.
    speak() : String {
        "Sonido de mamifero"
    };

    is_mammal() : Bool {
        has_fur
    };
};

-- -----------------------------------------------------------------------------
-- 3. SUBCLASE HOJA: Dog hereda de Mammal (y transitivamente de Animal e IO)
-- -----------------------------------------------------------------------------
class Dog inherits Mammal {
    breed : String;

    -- LUGAR 3 DE SELF_TYPE: Como tipo declarado de un atributo.
    -- Significa que este atributo guardará un objeto del mismo tipo dinámico que la clase.
    best_friend : SELF_TYPE;

    set_breed(b : String) : SELF_TYPE {
        {
            breed <- b;
            self;
        }
    };

    -- Override válido de speak() con retorno idéntico String:
    speak() : String {
        "Guau guau!"
    };

    play_fetch() : String {
        "Pelota atrapada!"
    };
};

-- -----------------------------------------------------------------------------
-- 4. OTRA SUBCLASE HOJA: Cat hereda de Mammal
-- -----------------------------------------------------------------------------
class Cat inherits Mammal {
    lives_left : Int <- 9;

    speak() : String {
        "Miau!"
    };
};

-- -----------------------------------------------------------------------------
-- 5. CLASE MAIN OBLIGATORIA
-- -----------------------------------------------------------------------------
-- Regla [Main]: Todo programa COOL debe contener una clase llamada 'Main'
-- con un método 'main()' que no reciba argumentos.
class Main inherits IO {
    dog : Dog;
    cat : Cat;
    anim : Animal;

    main() : Object {
        -- LUGAR 4 DE SELF_TYPE: Como tipo declarado en una ligadura 'let'.
        let count : Int <- 0,
            greeting : String <- "Iniciando pruebas semanticas de COOL...",
            copy_of_main : SELF_TYPE <- new SELF_TYPE
        in {
            -- Invocación de método de IO con concatenación de String:
            out_string(greeting.concat("\n"));

            -- 1. Instanciación e invocación encadenada con SELF_TYPE:
            dog <- (new Dog).set_breed("Labrador");
            dog.init("Firulais", 5);

            cat <- new Cat;
            cat.init("Michi", 3);

            -- 2. POLIMORFISMO Y LEAST UPPER BOUND (LUB) EN CONDICIONALES:
            -- Regla [If]: El tipo de 'if e1 then e2 else e3 fi' es lub(tipo(e2), tipo(e3)).
            -- Aquí: tipo(dog) = Dog, tipo(cat) = Cat.
            -- ancestros(Dog) = [Dog, Mammal, Animal, IO, Object]
            -- ancestros(Cat) = [Cat, Mammal, Animal, IO, Object]
            -- lub(Dog, Cat) = Mammal.
            -- Como Mammal <= Mammal, la asignación a 'chosen_animal' es totalmente válida.
            let chosen_animal : Mammal <- 
                if 2 < 3 then
                    dog
                else
                    cat
                fi
            in {
                out_string("Animal elegido: ");
                out_string(chosen_animal.get_name());
                out_string("\nSonido: ");
                out_string(chosen_animal.speak());
                out_string("\n");
            };

            -- 3. DESPACHO ESTÁTICO LEGAL (@):
            -- Regla [Static-Dispatch]: e@T.m(...) busca el método 'm' en la clase 'T'.
            -- Requisito estático: tipo(e) <= T. Aquí tipo(dog) = Dog <= Mammal.
            -- Invoca Mammal::speak() en lugar del método dinámico Dog::speak().
            out_string("Despacho estatico de Dog como Mammal: ");
            out_string(dog@Mammal.speak());
            out_string("\n");

            -- 4. BUCLE WHILE:
            -- Regla [Loop]: El tipo estático de 'while e1 loop e2 pool' es SIEMPRE Object.
            while count < 3 loop
                {
                    out_string("Contador en loop: ");
                    out_int(count);
                    out_string("\n");
                    count <- count + 1;
                }
            pool;

            -- 5. SENTENCIA CASE CON RAMAS DISYUNTAS:
            -- Regla [Case]: Cada rama debe tener un tipo distinto, no puede ser SELF_TYPE,
            -- y el tipo total del case es el LUB de los tipos de los cuerpos de cada rama.
            anim <- dog;
            let description : String <- 
                case anim of
                    d : Dog => d.play_fetch();
                    c : Cat => c.speak();
                    m : Mammal => m.speak();
                    a : Animal => a.get_name();
                    o : Object => "Objeto generico";
                esac
            in {
                out_string("Resultado del case: ");
                out_string(description);
                out_string("\n");
            };

            -- 6. OPERADOR ISVOID Y NEGACIÓN LÓGICA (not):
            -- Regla [IsVoid]: 'isvoid e' siempre tiene tipo estático Bool.
            -- Regla [Comp]: 'not e' requiere que 'e' sea Bool y retorna Bool.
            if not isvoid dog then
                out_string("El perro no es void!\n")
            else
                out_string("El perro es void!\n")
            fi;

            -- 7. MÉTODOS HEREDADOS DE Object (copy y type_name):
            -- 'copy()' retorna SELF_TYPE, que en anim conforma con Animal.
            -- 'type_name()' retorna el String con el nombre dinámico de la clase.
            let clone : Animal <- anim.copy() in {
                out_string("Tipo dinamico clonado: ");
                out_string(clone.type_name());
                out_string("\n");
            };

            out_string("Todas las verificaciones semanticas de good.cl completadas con exito!\n");
        }
    };
};
