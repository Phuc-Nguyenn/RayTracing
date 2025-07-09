![image](https://github.com/user-attachments/assets/fc6836eb-7d35-49ea-a99b-96536ae3ff24)
## Features
- fast progressive rendering
- bounding volume heirachy data structure to speed up ray intersection tests
- post processing bloom
- materials: glass, specular, metallic, lambertian
- record camera movements, render, and create a high quality video


## Installation

To set up the required dependencies for this project, install the following libraries:

```bash
sudo apt-get install libgl1-mesa-dev
sudo apt install libglew-dev
sudo apt-get install libglfw3-dev
```

## Build and Run
To build and run the project:
```cmd
bash build_and_run.sh
```

## Run only
To run the project after it has been built:
```cmd
bash run.sh
```

## Controls

Use the following controls to interact with the application:

- **W/A/S/D**: Translate the camera or object in the respective directions.
- **Arrow Keys**: Adjust the look direction.
- **Control**: Descend vertically.
- **Space**: Ascend vertically.
- hold **Left Shift**: speed up movement
- **Esc**: Exit the application.

## View Mode Controls
- **~**: view bounding boxes
- **num1**: view normals
- **num2**: 2 max ray bounces
- **num3**: 4 max ray bounces
- **num4**: 8 max ray bounces
- **num5**: 16 max ray bounces
- **num6**: 32 max ray bounces

## Recording Controls
- **R**: toggle recording on/off
- **P**: play/ start rendering the recording

## More Captures:
![image](https://github.com/user-attachments/assets/8bbec4fa-34c2-464c-8702-77ffb24d3563)
![image](https://github.com/user-attachments/assets/32d3fa2b-7e7a-4ac1-9689-d586de251fe0)

