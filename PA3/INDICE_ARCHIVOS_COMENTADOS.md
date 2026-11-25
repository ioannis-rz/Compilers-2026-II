# Índice de Archivos Comentados Línea por Línea
## Laboratorio 03: Analizador Semántico para COOL (PA4)
### Compiladores 2026-II · Universidad Nacional de Colombia

Este directorio contiene la versión original de entrega y una versión paralela **totalmente comentada línea por línea** de cada uno de los archivos de código del laboratorio. Estas versiones comentadas han sido diseñadas como material de estudio exhaustivo para la sustentación oral ante el profesor y monitores.

---

### Tabla de Archivos y Correspondencias

| Archivo Original | Archivo Comentado Línea por Línea | Descripción del Contenido |
| :--- | :--- | :--- |
| [`semant.h`](file:///c:/Users/juanm/.gemini/antigravity-ide/scratch/Nueva%20carpeta/PA3-entrega/semant.h) | [`semant_comentado.h`](file:///c:/Users/juanm/.gemini/antigravity-ide/scratch/Nueva%20carpeta/PA3-entrega/semant_comentado.h) | Interfaz pública de `ClassTable`, estructuras de metadatos `MethodInfo` y `AttrInfo`, métodos de tipado (`conforms`, `lub`), y firmas de las 7 fases del compilador. |
| [`semant.cc`](file:///c:/Users/juanm/.gemini/antigravity-ide/scratch/Nueva%20carpeta/PA3-entrega/semant.cc) | [`semant_comentado.cc`](file:///c:/Users/juanm/.gemini/antigravity-ide/scratch/Nueva%20carpeta/PA3-entrega/semant_comentado.cc) | Implementación de las 7 fases del pipeline semántico, registro de clases básicas (`Object`, `IO`, `Int`, `Bool`, `String`), validación de herencia y ciclos, comprobación de `Main.main()`, y tipado estático recursivo de los 24 nodos de expresión del AST. |
| [`cool-tree.handcode.h`](file:///c:/Users/juanm/.gemini/antigravity-ide/scratch/Nueva%20carpeta/PA3-entrega/cool-tree.handcode.h) | [`cool-tree.handcode_comentado.h`](file:///c:/Users/juanm/.gemini/antigravity-ide/scratch/Nueva%20carpeta/PA3-entrega/cool-tree.handcode_comentado.h) | Macros de inyección de C++ de Stanford CS143 (`Program_EXTRAS`, `Class__EXTRAS`, `Expression_SHARED_EXTRAS`), método virtual `type_check`, atributos de tipo (`Symbol type`), y accesores `get_type()` / `set_type()`. |
| [`good.cl`](file:///c:/Users/juanm/.gemini/antigravity-ide/scratch/Nueva%20carpeta/PA3-entrega/good.cl) | [`good_comentado.cl`](file:///c:/Users/juanm/.gemini/antigravity-ide/scratch/Nueva%20carpeta/PA3-entrega/good_comentado.cl) | Programa COOL válido exhaustivo. Explica las 4 posiciones legales de `SELF_TYPE`, la regla de Least Upper Bound (LUB) en condicionales y case, despacho dinámico vs estático (`@`), y bucles `while`. |
| [`bad.cl`](file:///c:/Users/juanm/.gemini/antigravity-ide/scratch/Nueva%20carpeta/PA3-entrega/bad.cl) | [`bad_comentado.cl`](file:///c:/Users/juanm/.gemini/antigravity-ide/scratch/Nueva%20carpeta/PA3-entrega/bad_comentado.cl) | Batería de pruebas negativas. Detalla cada error intencional mapeado a las categorías del enunciado (A.1 a E.9), la regla formal violada y el mensaje exacto que emite `semant.cc`. |

---

### Documentación Adicional para la Sustentación
- [`EXPLICACION_DETALLADA.md`](file:///c:/Users/juanm/.gemini/antigravity-ide/scratch/Nueva%20carpeta/PA3-entrega/EXPLICACION_DETALLADA.md): Guía teórica completa con el banco de **28 preguntas y respuestas frecuentes** para la sustentación oral del 26 de noviembre.
- [`README.md`](file:///c:/Users/juanm/.gemini/antigravity-ide/scratch/Nueva%20carpeta/PA3-entrega/README.md): Guía de comandos para clonar, compilar y ejecutar `./pa3-grading.pl` en la Máquina Virtual.
- [`verify_semant.py`](file:///c:/Users/juanm/.gemini/antigravity-ide/scratch/Nueva%20carpeta/PA3-entrega/verify_semant.py): Script de validación local (85/85 checks, 100% PASS).
