//
//  wu_collage_advanced.cpp
//  wu_collage_advanced
//
//  Created by Zhipeng Wu on 8/18/12.
//  Copyright (c) 2012 Zhipeng Wu. All rights reserved.
//

#include "wu_collage_advanced.h"
#include <math.h>
#include <fstream>
#include <iostream>

bool less_than(AlphaUnit m, AlphaUnit n) {
  return m.alpha_ < n.alpha_;
}

// Create collage.
bool CollageAdvanced::CreateCollage(float expect_alpha) {
  assert(expect_alpha > 0);
  
  // Step 1: Sort the image_alpha_ vector fot generate guided binary tree.
  std::sort(image_alpha_vec_.begin(), image_alpha_vec_.end(), less_than);
  // Step 2: Generate a guided binary tree by using divide-and-conquer.
  GenerateTree(expect_alpha);
  // Step 3: Calculate the actual aspect ratio for the generated collage.
  canvas_alpha_ = CalculateAlpha(tree_root_);
  canvas_width_ = static_cast<int>(canvas_height_ * canvas_alpha_);
  // Step 4: Get the position for the nodes in the binary tree.
  tree_root_->position_.x = 0;
  tree_root_->position_.y = 0;
  tree_root_->position_.height = canvas_height_;
  tree_root_->position_.width = canvas_width_;
  CalculatePositions(tree_root_->left_child_);
  CalculatePositions(tree_root_->right_child_);
  return true;
}

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
bool CollageAdvanced::CreateCollage(float expect_alpha, float thresh) {
};

// After calling CreateCollage() and FastAdjust(), call this function to save result
// collage to a image file specified by out_put_image_path.
cv::Mat CollageAdvanced::OutputCollageImage() const {
  // Traverse tree_leaves_ vector. Resize tile image and paste it on the canvas.
  assert(canvas_alpha_ != -1);
  assert(canvas_width_ != -1);
  cv::Mat canvas(cv::Size(canvas_width_, canvas_height_), image_vec_[0].type());
  for (int i = 0; i < image_num_; ++i) {
    int img_ind = tree_leaves_[i]->image_index_;
    cv::Rect pos = tree_leaves_[i]->position_;
    cv::Mat roi(canvas, pos);
    assert(image_vec_[0].type() == image_vec_[img_ind].type());
    cv::Mat resized_img(pos.height, pos.width, image_vec_[i].type());
    cv::resize(image_vec_[img_ind], resized_img, resized_img.size());
    resized_img.copyTo(roi);
  }
  return canvas;
}

// After calling CreateCollage(), call this function to save result
// collage to a html file specified by out_put_html_path.
bool CollageAdvanced::OutputCollageHtml(const std::string output_html_path) {
  assert(canvas_alpha_ != -1);
  assert(canvas_width_ != -1);
  std::ofstream output_html(output_html_path.c_str());
  if (!output_html) {
    std::cout << "Error: OutputCollageHtml" << std::endl;
  }
  
  output_html << "<!DOCTYPE html>\n";
  output_html << "<html>\n";
  output_html << "<h1 style=\"text-align:left\">\n";
  output_html << "\tImage Collage\n";
  output_html << "</h1>\n";
  output_html << "<hr //>\n";
  output_html << "\t<body>\n";
  output_html << "\t\t<div style=\"position:absolute;\">\n";
  for (int i = 0; i < image_num_; ++i) {
    int img_ind = tree_leaves_[i]->image_index_;
    output_html << "\t\t\t<a href=\"";
    output_html << image_path_vec_[img_ind];
    output_html << "\">\n";
    output_html << "\t\t\t\t<img src=\"";
    output_html << image_path_vec_[img_ind];
    output_html << "\" style=\"position:absolute; width:";
    output_html << tree_leaves_[i]->position_.width;
    output_html << "px; height:";
    output_html << tree_leaves_[i]->position_.height;
    output_html << "px; left:";
    output_html << tree_leaves_[i]->position_.x;
    output_html << "px; top:";
    output_html << tree_leaves_[i]->position_.y;
    output_html << "px;\">\n";
    output_html << "\t\t\t</a>\n";
  }
  output_html << "\t\t</div>\n";
  output_html << "\t</body>\n";
  output_html << "</html>";
  output_html.close();
  return true;
}

// Private member functions:
// The images are stored in the image list, one image path per row.
// This function reads the images into image_vec_ and their aspect
// ratios into image_alpha_vec_.
bool CollageAdvanced::ReadImageList(std::string input_image_list) {
  std::ifstream input_list(input_image_list.c_str());
  if (!input_list) {
    std::cout << "Error: ReadImageList()" << std::endl;
    return false;
  }
  int index = 0;
  while (!input_list.eof()) {
    std::string img_path;
    std::getline(input_list, img_path);
    // std::cout << img_path <<std::endl;
    cv::Mat img = cv::imread(img_path.c_str());
    image_vec_.push_back(img);
    AlphaUnit new_unit;
    new_unit.image_ind_ = index;
    new_unit.alpha_ = static_cast<float>(img.cols) / img.rows;
    new_unit.alpha_recip_ = static_cast<float>(img.rows) / img.cols;
    image_alpha_vec_.push_back(new_unit);
    image_path_vec_.push_back(img_path);
    ++index;
  }
  input_list.close();
  return true;
}

// Recursively calculate aspect ratio for all the inner nodes.
// The return value is the aspect ratio for the node.
float CollageAdvanced::CalculateAlpha(TreeNode* node) {
  if (!node->is_leaf_) {
    float left_alpha = CalculateAlpha(node->left_child_);
    float right_alpha = CalculateAlpha(node->right_child_);
    if (node->split_type_ == 'v') {
      node->alpha_ = left_alpha + right_alpha;
      return node->alpha_;
    } else if (node->split_type_ == 'h') {
      node->alpha_ = (left_alpha * right_alpha) / (left_alpha + right_alpha);
      return node->alpha_;
    } else {
      std::cout << "Error: CalculateAlpha" << std::endl;
      return -1;
    }
  } else {
    // This is a leaf node, just return the image's aspect ratio.
    return node->alpha_;
  }
}

// Top-down Calculate the image positions in the colage.
bool CollageAdvanced::CalculatePositions(TreeNode* node) {
  // Step 1: calculate height & width.
  if (node->parent_->split_type_ == 'v') {
    // Vertical cut, height unchanged.
    node->position_.height = node->parent_->position_.height;
    if (node->child_type_ == 'l') {
      node->position_.width = static_cast<int>
      (node->position_.height * node->alpha_);
    } else if (node->child_type_ == 'r') {
      node->position_.width = node->parent_->position_.width -
      node->parent_->left_child_->position_.width;
    } else {
      std::cout << "Error: CalculatePositions step 0" << std::endl;
      return false;
    }
  } else if (node->parent_->split_type_ == 'h') {
    // Horizontal cut, width unchanged.
    node->position_.width = node->parent_->position_.width;
    if (node->child_type_ == 'l') {
      node->position_.height = static_cast<int>
      (node->position_.width / node->alpha_);
    } else if (node->child_type_ == 'r') {
      node->position_.height = node->parent_->position_.height -
      node->parent_->left_child_->position_.height;
    }
  } else {
    std::cout << "Error: CalculatePositions step 1" << std::endl;
    return false;
  }
  
  // Step 2: calculate x & y.
  if (node->child_type_ == 'l') {
    // If it is left child, use its parent's x & y.
    node->position_.x = node->parent_->position_.x;
    node->position_.y = node->parent_->position_.y;
  } else if (node->child_type_ == 'r') {
    if (node->parent_->split_type_ == 'v') {
      // y (row) unchanged, x (colmn) changed.
      node->position_.y = node->parent_->position_.y;
      node->position_.x = node->parent_->position_.x +
      node->parent_->position_.width -
      node->position_.width;
    } else if (node->parent_->split_type_ == 'h') {
      // x (column) unchanged, y (row) changed.
      node->position_.x = node->parent_->position_.x;
      node->position_.y = node->parent_->position_.y +
      node->parent_->position_.height -
      node->position_.height;
    } else {
      std::cout << "Error: CalculatePositions step 2 - 1" << std::endl;
    }
  } else {
    std::cout << "Error: CalculatePositions step 2 - 2" << std::endl;
    return false;
  }
  
  // Calculation for children.
  if (node->left_child_) {
    bool success = CalculatePositions(node->left_child_);
    if (!success) return false;
  }
  if (node->right_child_) {
    bool success = CalculatePositions(node->right_child_);
    if (!success) return false;
  }
  return true;
}


// Release the memory for binary tree.
void CollageAdvanced::ReleaseTree(TreeNode* node) {
  if (node == NULL) return;
  if (node->left_child_) ReleaseTree(node->left_child_);
  if (node->right_child_) ReleaseTree(node->right_child_);
  delete node;
}

void CollageAdvanced::GenerateTree(float expect_alpha) {
  if (tree_root_) ReleaseTree(tree_root_);
  tree_leaves_.clear();
  // Copy image_alpha_vec_ for local computation.
  std::vector<AlphaUnit> local_alpha;
  for (int i = 0; i < image_alpha_vec_.size(); ++i) {
    local_alpha.push_back(image_alpha_vec_[i]);
  }
  
  // Generate a new tree by using divide-and-conquer.
  tree_root_ = GuidedTree(tree_root_, 'N', expect_alpha, image_num_, local_alpha);
  // After guided tree generation, all the images have been dispatched to leaves.
  assert(local_alpha.size() == 0);
  return;
}

// Divide-and-conquer tree generation.
TreeNode* CollageAdvanced::GuidedTree(TreeNode* parent,
                                      char child_type,
                                      float expect_alpha,
                                      int image_num,
                                      std::vector<AlphaUnit> alpha_array) {
  if (alpha_array.size() == 0) {
    std::cout << "Error: GuidedTree" << std::endl;
    return NULL;
  }
  
  // Create a new TreeNode.
  TreeNode* node = new TreeNode();
  node->parent_ = parent;
  node->child_type_ = child_type;
  
  if (image_num == 1) {
    // Set the new node.
    node->is_leaf_ = true;
    // Find the best fit aspect ratio.
    
    // Since this node is a leaf node, we have to set the followings.
    node->alpha_ =
    node->image_index_ = 
  } else if (image_num == 2) {
    // Set the new node.
    node->is_leaf_ = false;
    
    // Find the best fit aspect ratio with two nodes.
    // As well as the split type for node.
    
    node->split_type_ =
    
    TreeNode* l_child new TreeNode();
    l_child.child_type_ = 'l';
    l_chil.parent_ = node;
    l_child.alpha_ =
    l_child.is_leaf_ = true;
    l_child->image_index_ =
    node->left_child_ = l_child;
    
    TreeNode* r_child = new TreeNode();
    r_child.child_type_ = 'r';
    r_chil.parent_ = node;
    r_child.alpha_ =
    r_child.is_leaf_ = true;
    r_child->image_index_ =
    node->right_child_ = r_child;
  } else {
    node->is_leaf_ = false;
    float new_exp_alpha = 0;
    // Random split type.
    int v_h = random(2);
    if (v_h == 1) {
      node->split_type_ = 'v';
      new_exp_alpha = expect_alpha / 2;
    } else {
      node->split_type_ = 'h';
      new_exp_alpha = expect_alpha * 2;
    }
    int new_img_num_1 = static_cast<int>(image_num / 2);
    int new_img_num_2 = image_num - new_img_num_1;
    if (new_img_num_1 > 0) {
      TreeNode* l_child = new TreeNode();
      l_child = GuidedTree(node, 'l', new_exp_alpha, new_img_num_1, alpha_array);
      node->left_child_ = l_child;
    }
    if (new_img_num_2 > 0) {
      TreeNode* r_child = new TreeNode();
      r_child = GuidedTree(node, 'r', new_exp_alpha, new_img_num_2, alpha_array);
      node->right_child_ = r_child;
    }
  }
  return node;
  
}