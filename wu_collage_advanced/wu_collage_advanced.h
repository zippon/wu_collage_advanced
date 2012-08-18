//
//  wu_collage_advanced.h
//  wu_collage_advanced
//
//  Created by Zhipeng Wu on 8/18/12.
//  Copyright (c) 2012 Zhipeng Wu. All rights reserved.
//

#ifndef __wu_collage_advanced__wu_collage_advanced__
#define __wu_collage_advanced__wu_collage_advanced__

#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <time.h>
#define random(x) (rand() % x)

class TreeNode {
public:
  TreeNode() {
    child_type_ = 'N';
    split_type_ = 'N';
    is_leaf_ = true;
    alpha_ = 0;
    alpha_expect_ = 0;
    position_ = cv::Rect(0, 0, 0, 0);
    image_index_ = -1;
    left_child_ = NULL;
    right_child_ = NULL;
    parent_ = NULL;
  }
  char child_type_;      // Is this node left child "l" or right child "r".
  char split_type_;      // If this node is a inner node, we set 'v' or 'h', which indicate
  // vertical cut or horizontal cut.
  bool is_leaf_;         // Is this node a leaf node or a inner node.
  float alpha_expect_;   // If this node is a leaf, we set expected aspect ratio of this node.
  float alpha_;          // If this node is a leaf, we set actual aspect ratio of this node.
  cv::Rect position_;    // The position of the node on canvas.
  int image_index_;      // If this node is a leaf, it is related with a image.
  TreeNode* left_child_;
  TreeNode* right_child_;
  TreeNode* parent_;
};


class AlphaUnit {
public:
  int image_ind_;        // The related image index.
  float alpha_;          // Aspect ratio value.
  float alpha_recip_;    // Reciprocal sapect ratio value.
};

// Collage with pre-defined aspect ratio
class CollageAdvanced {
public:
  // Constructors.
  // We need to let the user decide the canvas height.
  // Since the aspect ratio will be calculate by our program, we can compute
  // canvas width accordingly.
  CollageAdvanced (const std::string input_image_list, int canvas_height) {
    ReadImageList(input_image_list);
    canvas_height_ = canvas_height;
    canvas_alpha_ = -1;
    canvas_width_ = -1;
    image_num_ = static_cast<int>(image_vec_.size());
    srand(static_cast<unsigned>(time(0)));
  }
  ~CollageAdvanced() {
    ReleaseTree(tree_root_);
    image_vec_.clear();
    image_alpha_vec_.clear();
    image_path_vec_.clear();
  }
  
  // Create collage.
  bool CreateCollage(float expect_alpha);
  
  // If we use CreateCollage, the generated collage may have strange aspect ratio such as
  // too big or too small, which seems to be difficult to be shown. We let the user to
  // input their expected aspect ratio and fast adjust to make the result aspect ratio
  // close to the user defined one.
  // The thresh here controls the closeness between the result aspect ratio and the expect
  // aspect ratio. e.g. expect_alpha is 1, thresh is 2. The result aspect ratio is around
  // [1 / 2, 1 * 2] = [0.5, 2].
  // We also define MAX_ITER_NUM = 100,
  // If max iteration number is reached and we cannot find a good result aspect ratio,
  // this function returns false.
  bool CreateCollage(float expect_alpha, float thresh = 1.2);
  
  // Output collage into a single image.
  cv::Mat OutputCollageImage() const;
  // Output collage into a html page.
  bool OutputCollageHtml (const std::string output_html_path);
  
  // Accessors:
  int image_num() const {
    return image_num_;
  }
  int canvas_height() const {
    return canvas_height_;
  }
  int canvas_width() const {
    return canvas_width_;
  }
  float canvas_alpha() const {
    return canvas_alpha_;
  }
  
private:
  // Read input images from image list.
  bool ReadImageList(std::string input_image_list);
  // Generate an initial full balanced binary tree with image_num_ leaf nodes.
  float CalculateAlpha(TreeNode* node);
  // Top-down Calculate the image positions in the colage.
  bool CalculatePositions(TreeNode* node);
  // Clean and release the binary_tree.
  void ReleaseTree(TreeNode* node);
  // Guided binary tree generation.
  void GenerateTree(float expect_alpha);
  // Divide-and-conquer tree generation.
  TreeNode* GuidedTree(TreeNode* parent,ß
                       char child_type,
                       float expect_alpha,
                       int image_num,
                       std::vector<AlphaUnit> alpha_array);
  
  // Vector containing input image paths.
  std::vector<std::string> image_path_vec_;
  // Vector containing input images.
  std::vector<cv::Mat> image_vec_;
  // Vector containing input images' aspect ratios.
  std::vector<AlphaUnit> image_alpha_vec_;
  // Vector containing leaf nodes of the tree.
  std::vector<TreeNode*> tree_leaves_;
  // Number of images in the collage. (number of leaf nodes in the tree)
  int image_num_;
  // Full balanced binary for collage generation.
  TreeNode* tree_root_;
  // Canvas height, this is decided by the user.
  int canvas_height_;
  // Canvas aspect ratio, return by CalculateAspectRatio ().
  float canvas_alpha_;
  // Canvas width, this is computed according to canvas_aspect_ratio_.
  int canvas_width_;
  
};

#endif /* defined(__wu_collage_advanced__wu_collage_advanced__) */
