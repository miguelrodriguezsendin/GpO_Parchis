# Graficos por ordenador
El proyecto completo está en otro repo, hacer git clone --recursive https://github.com/juanpebm/gpo-framework.git
Borrar todas las clases dentro de src menos GPO_aux.cpp
Quitarlo también de CMakeLists.txt
Crear clase parchis_inicial.cpp y copiar el contenido de este repo, ademas de añadirlo a CMakeLists.txt
Crear un build en GPO-FRAMEWORK>build y en él ejecutar los comandos:
- Para construir el proyecto: cmake -DCMAKE_POLICY_VERSION_MINIMUM="3.5" ..
- Para compilar: cmake --build . --config Debug

Ya está, ahora tendremos que buscar el ejecutable del parchís en build\bin\Debug e ir compilando cada vez que cambiemos código.