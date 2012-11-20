//
//  main.cpp
//  wu_collage_advanced
//
//  Created by Zhipeng Wu on 8/17/12.
//  Copyright (c) 2012 Zhipeng Wu. All rights reserved.
//

#include "wu_collage_advanced.h"
#include <iostream>
#include <time.h>
#include <stdlib.h>

int main(int argc, const char * argv[]) {
  std::cout << "Well come to \"Collage Advanced\"" << std::endl << std::endl;;
  
  if (argc != 2) {
    std::cout << "Error number of input arguments" << std::endl;
    return 0;
  }
  std::string image_list(argv[1]);
  int canvas_width = 0;
  while ((canvas_width < 100) || (canvas_width > 2000)) {
    std::cout << "canvas_width [100, 2000]: ";
    std::cin >> canvas_width;
  }
  float expect_alpha = 0;
  while ((expect_alpha < 0.1) || (expect_alpha > 10)) {
    std::cout << "expect_alpha: [0.1, 10]: ";
    std::cin >> expect_alpha;
  }
  
  clock_t start, end;
  CollageAdvanced my_collage(image_list, canvas_width);
  
  start = clock();
  //bool success = my_collage.CreateCollage();
  int total_tree_generation = 0;
  int total_adjust_iteration = 0;
  int success = my_collage.CreateCollage(expect_alpha, 1.1,
                                         total_tree_generation,
                                         total_adjust_iteration);
  if (success == -1) {
    return -1;
  }
  end = clock();
  cv::Mat canvas = my_collage.OutputCollageImage();
  int canvas_height = my_collage.canvas_height();
  float canvas_alpha = my_collage.canvas_alpha();
  std::cout << "Tree re-generation number: " << total_tree_generation << std::endl;
  std::cout << "Tree adjust number: " << total_adjust_iteration << std::endl;
  std::cout << "canvas_height: " << canvas_height << std::endl;
  std::cout << "canvas_alpha: " << canvas_alpha << std::endl;
  std::cout << "processing time: " << (end - start) * 1000000 / CLOCKS_PER_SEC
  << " us (10e-6 s)" << std::endl;
  std::string html_save_path = "/tmp/collage_result.html";
  my_collage.OutputCollageHtml(html_save_path);
  cv::imshow("Collage", canvas);
  cv::waitKey();
  return 0;
}

