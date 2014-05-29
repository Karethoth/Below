# Below
### Dependencies:
 - GL
 - GLEW
 - GLM
 - boost-system
 - SDL2


### Installing dependencies for debian based distributions:

#### GL, GLEW and boost-system:
```sh
sudo apt-get install libglew-dev libboost-system-dev
```

#### Quick GLM installation script:
```sh
mkdir tmpGLM
cd tmpGLM
wget https://github.com/g-truc/glm/archive/0.9.5.zip
unzip 0.9.5.zip
sudo cp -R glm-0.9.5/glm /usr/include/
cd ..
rm -rf tmpGLM

```

#### SDL2
If your repositories happen to provide libsdl2-dev, then:
```sh
sudo apt-get install libsdl2-dev
```

Otherwise, get the latest source from [here](http://www.libsdl.org/download-2.0.php) and then compile and install it.

