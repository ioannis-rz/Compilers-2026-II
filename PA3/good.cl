(*
 * good.cl - Programa COOL sintáctica y semánticamente correcto
 * Diseñado para validar exhaustivamente el analizador semántico estático (PA4/Laboratorio 03).
 *
 * Características evaluadas:
 * 1. Jerarquía de herencia múltiple y válida (Animal -> Mammal -> Dog).
 * 2. Sobreescritura (override) válida de métodos con firmas idénticas.
 * 3. Uso legal de SELF_TYPE en los 4 lugares permitidos:
 *    - Tipo de retorno de método.
 *    - Tipo de una variable let.
 *    - Tipo de un atributo.
 *    - new SELF_TYPE.
 * 4. Despachos dinámicos y despachos estáticos (@).
 * 5. Expresiones condicionales (if) con cómputo de Least Upper Bound (LUB).
 * 6. Bucles while (cuyo tipo es siempre Object).
 * 7. Bloques de expresiones { ... } evaluados a la última expresión.
 * 8. Expresiones let con múltiples variables, alcances anidados y sombreado (shadowing).
 * 9. Sentencia case con ramas disyuntas no repetidas y cómputo de LUB.
 * 10. Operaciones aritméticas, lógicas y comparaciones válidas.
 * 11. Invocación de métodos integrados de Object, IO y String.
 * 12. Clase Main con método main() sin parámetros.
 *)

class Animal inherits IO {
    name : String;
    age  : Int;

    init(n : String, a : Int) : SELF_TYPE {
        {
            name <- n;
            age <- a;
            self;
        }
    };

    get_name() : String {
        name
    };

    get_age() : Int {
        age
    };

    speak() : String {
        "Sonido generico de animal"
    };

    clone_self() : SELF_TYPE {
        new SELF_TYPE
    };
};

class Mammal inherits Animal {
    has_fur : Bool <- true;

    speak() : String {
        "Sonido de mamifero"
    };

    is_mammal() : Bool {
        has_fur
    };
};

class Dog inherits Mammal {
    breed : String;
    best_friend : SELF_TYPE;

    set_breed(b : String) : SELF_TYPE {
        {
            breed <- b;
            self;
        }
    };

    speak() : String {
        "Guau guau!"
    };

    play_fetch() : String {
        "Pelota atrapada!"
    };
};

class Cat inherits Mammal {
    lives_left : Int <- 9;

    speak() : String {
        "Miau!"
    };
};

class Main inherits IO {
    dog : Dog;
    cat : Cat;
    anim : Animal;

    main() : Object {
        let count : Int <- 0,
            greeting : String <- "Iniciando pruebas semanticas de COOL...",
            copy_of_main : SELF_TYPE <- new SELF_TYPE
        in {
            out_string(greeting.concat("\n"));

            -- 1. Instanciación e inicialización con SELF_TYPE
            dog <- (new Dog).set_breed("Labrador");
            dog.init("Firulais", 5);

            cat <- new Cat;
            cat.init("Michi", 3);

            -- 2. Polimorfismo y LUB en condicionales (LUB(Dog, Cat) = Mammal)
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

            -- 3. Despacho estático legal (@Mammal)
            out_string("Despacho estatico de Dog como Mammal: ");
            out_string(dog@Mammal.speak());
            out_string("\n");

            -- 4. Bucle while y evaluación a Object
            while count < 3 loop
                {
                    out_string("Contador en loop: ");
                    out_int(count);
                    out_string("\n");
                    count <- count + 1;
                }
            pool;

            -- 5. Expresión Case con ramas disyuntas
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

            -- 6. Verificación de comparaciones y métodos integrados
            if not isvoid dog then
                out_string("El perro no es void!\n")
            else
                out_string("El perro es void!\n")
            fi;

            -- 7. Copia e introspección mediante métodos de Object
            let clone : Animal <- anim.copy() in {
                out_string("Tipo dinamico clonado: ");
                out_string(clone.type_name());
                out_string("\n");
            };

            out_string("Todas las verificaciones semanticas de good.cl completadas con exito!\n");
        }
    };
};
