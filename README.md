# Short Description

![collage](http://farm9.staticflickr.com/8348/8201228429_2869581c82_b.jpg)
##Overview
The files in this folder simply allows you to create an image collage with a set of input images. The collage algorithm features in the following points:

1. **Fast**: Given a set of input images, we can generate photo collage on-the-fly, which is particular suitable for real-time applications such as image retrieval service, online games, and human-computer interaction. According to experimental results, it costs less than 0.5ms for a 100-input-photo collage generation (excluding the time for image reading), and less than 0.1ms for 20-input-photo collage.
2. **Compact**: We allow the user to personalize the size of collage by setting canvas height and width. 
3. **Content-reserved**: we assure to fully reserve the visual content of input images. Although these photos can be stretched, their aspect ratios are strictly kept, and there is no cropping as well as changing of orientations.
##Build
To build the binary, you need to pre-install [OpenCV](http://opencv.org/) on your machine.`gcc -o collage_main main.cpp wu_collage_advanced.cpp -I path/to/your/opencv/include -L path/to/your/opencv/lib -lopencv_highgui -lopencv_core -lopencv_imgproc`
##Test
The binary requires a list which contains a set of images. A typical example for the input images and lists can be found in the ‘*test*’ folder. To run the binary:
`./collage_main the/path/to/your/image/list`
Then, you are required to enter the expected **width** and **aspect ratio** for the collage canvas.
##Contact
<zippon.wu@gmail.com>




