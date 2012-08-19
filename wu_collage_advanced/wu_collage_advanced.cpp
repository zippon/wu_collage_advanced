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
  tree_root_->position_.x_ = 0;
  tree_root_->position_.y_ = 0;
  tree_root_->position_.height_ = canvas_height_;
  tree_root_->position_.width_ = canvas_width_;
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
// this function returns -1.
int CollageAdvanced::CreateCollage(float expect_alpha, float thresh) {
  assert(thresh > 1);
  assert(expect_alpha > 0);
  tree_root_->alpha_expect_ = expect_alpha;
  float lower_bound = expect_alpha / thresh;
  float upper_bound = expect_alpha * thresh;
  int total_iter_counter = 1;
  int iter_counter = 1;
  int tree_gene_counter = 1;
  // Step 1: Sort the image_alpha_ vector fot generate guided binary tree.
  std::sort(image_alpha_vec_.begin(), image_alpha_vec_.end(), less_than);
  // Step 2: Generate a guided binary tree by using divide-and-conquer.
  GenerateTree(expect_alpha);
  // Step 3: Calculate the actual aspect ratio for the generated collage.
  canvas_alpha_ = CalculateAlpha(tree_root_);
  
  while ((canvas_alpha_ < lower_bound) || (canvas_alpha_ > upper_bound)) {
    // Call the following function to adjust the aspect ratio from top to down.
    tree_root_->alpha_expect_ = expect_alpha;
    AdjustAlpha(tree_root_, thresh);
    // Calculate actual aspect ratio again.
    canvas_alpha_ = CalculateAlpha(tree_root_);
    ++iter_counter;
    ++total_iter_counter;
    if (iter_counter > MAX_ITER_NUM) {
      std::cout << "*******************************" << std::endl;
      std::cout << "max iteration number reached..." << std::endl;
      std::cout << "*******************************" << std::endl << std::endl;
      // We should generate binary tree again
      iter_counter = 1;
      ++total_iter_counter;
      GenerateTree(expect_alpha);
      canvas_alpha_ = CalculateAlpha(tree_root_);
      ++tree_gene_counter;
      if (tree_gene_counter > MAX_TREE_GENE_NUM) {
        std::cout << "-------------------------------------------------------";
        std::cout << std::endl;
        std::cout << "WE HAVE DONE OUR BEST, BUT COLAAGE GENERATION FAILED...";
        std::cout << std::endl;
        std::cout << "-------------------------------------------------------";
        std::cout << std::endl;
        return -1;
      }
    }
  }
  // std::cout << "Canvas generation success!" << std::endl;
  std::cout << "Tree generation number is: " << tree_gene_counter << std::endl;
  std::cout << "Total iteration number is: " << total_iter_counter << std::endl;
  // After adjustment, set the position for all the tile images.
  canvas_width_ = static_cast<int>(canvas_height_ * canvas_alpha_);
  tree_root_->position_.x_ = 0;
  tree_root_->position_.y_ = 0;
  tree_root_->position_.height_ = canvas_height_;
  tree_root_->position_.width_ = canvas_width_;
  CalculatePositions(tree_root_->left_child_);
  CalculatePositions(tree_root_->right_child_);
  return total_iter_counter;
}

// After calling CreateCollage() and FastAdjust(), call this function to save result
// collage to a image file specified by out_put_image_path.
cv::Mat CollageAdvanced::OutputCollageImage() const {
  // Traverse tree_leaves_ vector. Resize tile image and paste it on the canvas.
  assert(canvas_alpha_ != -1);
  assert(canvas_width_ != -1);
  cv::Mat canvas(cv::Size(canvas_width_, canvas_height_), image_vec_[0].type());
  for (int i = 0; i < image_num_; ++i) {
    int img_ind = tree_leaves_[i]->image_index_;
    FloatRect pos = tree_leaves_[i]->position_;
    cv::Rect pos_cv(pos.x_, pos.y_, pos.width_, pos.height_);
    cv::Mat roi(canvas, pos_cv);
    assert(image_vec_[0].type() == image_vec_[img_ind].type());
    cv::Mat resized_img(pos_cv.height, pos_cv.width, image_vec_[i].type());
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
    output_html << tree_leaves_[i]->position_.width_;
    output_html << "px; height:";
    output_html << tree_leaves_[i]->position_.height_;
    output_html << "px; left:";
    output_html << tree_leaves_[i]->position_.x_;
    output_html << "px; top:";
    output_html << tree_leaves_[i]->position_.y_;
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
    node->position_.height_ = node->parent_->position_.height_;
    if (node->child_type_ == 'l') {
      node->position_.width_ = node->position_.height_ * node->alpha_;
    } else if (node->child_type_ == 'r') {
      node->position_.width_ = node->parent_->position_.width_ -
      node->parent_->left_child_->position_.width_;
    } else {
      std::cout << "Error: CalculatePositions step 0" << std::endl;
      return false;
    }
  } else if (node->parent_->split_type_ == 'h') {
    // Horizontal cut, width unchanged.
    node->position_.width_ = node->parent_->position_.width_;
    if (node->child_type_ == 'l') {
      node->position_.height_ = node->position_.width_ / node->alpha_;
    } else if (node->child_type_ == 'r') {
      node->position_.height_ = node->parent_->position_.height_ -
      node->parent_->left_child_->position_.height_;
    }
  } else {
    std::cout << "Error: CalculatePositions step 1" << std::endl;
    return false;
  }
  
  // Step 2: calculate x & y.
  if (node->child_type_ == 'l') {
    // If it is left child, use its parent's x & y.
    node->position_.x_ = node->parent_->position_.x_;
    node->position_.y_ = node->parent_->position_.y_;
  } else if (node->child_type_ == 'r') {
    if (node->parent_->split_type_ == 'v') {
      // y (row) unchanged, x (colmn) changed.
      node->position_.y_ = node->parent_->position_.y_;
      node->position_.x_ = node->parent_->position_.x_ +
      node->parent_->position_.width_ -
      node->position_.width_;
    } else if (node->parent_->split_type_ == 'h') {
      // x (column) unchanged, y (row) changed.
      node->position_.x_ = node->parent_->position_.x_;
      node->position_.y_ = node->parent_->position_.y_ +
      node->parent_->position_.height_ -
      node->position_.height_;
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
                                      int img_num,
                                      std::vector<AlphaUnit>& alpha_array) {
  if (alpha_array.size() == 0) {
    std::cout << "Error: GuidedTree 0" << std::endl;
    return NULL;
  }
  
  // Create a new TreeNode.
  TreeNode* node = new TreeNode();
  node->parent_ = parent;
  node->child_type_ = child_type;
  
  if (img_num == 1) {
    // Set the new node.
    node->is_leaf_ = true;
    // Find the best fit aspect ratio.
    bool success = FindOneImage(expect_alpha,
                                alpha_array,
                                node->image_index_,
                                node->alpha_);
    if (!success) {
      std::cout << "Error: GuidedTree 1" << std::endl;
      return NULL;
    }
    tree_leaves_.push_back(node);
  } else if (img_num == 2) {
    // Set the new node.
    node->is_leaf_ = false;
    TreeNode* l_child = new TreeNode();
    l_child->child_type_ = 'l';
    l_child->parent_ = node;
    l_child->is_leaf_ = true;
    node->left_child_ = l_child;
    
    TreeNode* r_child = new TreeNode();
    r_child->child_type_ = 'r';
    r_child->parent_ = node;
    r_child->is_leaf_ = true;
    node->right_child_ = r_child;
    // Find the best fit aspect ratio with two nodes.
    // As well as the split type for node.
    bool success = FindTwoImages(expect_alpha, alpha_array,
                                 node->split_type_,
                                 l_child->image_index_,
                                 l_child->alpha_,
                                 r_child->image_index_,
                                 r_child->alpha_);
    if (!success) {
      std::cout << "Error: GuidedTree 2" << std::endl;
      return NULL;
    }
    tree_leaves_.push_back(l_child);
    tree_leaves_.push_back(r_child);
  } else {
    node->is_leaf_ = false;
    float new_exp_alpha = 0;
    // Random split type.
    int v_h = random(2);
    if (expect_alpha > 10) v_h = 1;
    if (expect_alpha < 0.1) v_h = 0;
    if (v_h == 1) {
      node->split_type_ = 'v';
      new_exp_alpha = expect_alpha / 2;
    } else {
      node->split_type_ = 'h';
      new_exp_alpha = expect_alpha * 2;
    }
    int new_img_num_1 = static_cast<int>(img_num / 2);
    int new_img_num_2 = img_num - new_img_num_1;
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

// Find the best-match aspect ratio image in the given array.
// alpha_array is the array storing aspect ratios.
// find_img_ind is the best-match image index according to image_vec_.
// find_img_alpha is the best-match alpha value.
// After finding the best-match one, the AlphaUnit is removed from alpha_array,
// which means that we have dispatched one image with a tree leaf.
bool CollageAdvanced::FindOneImage(float expect_alpha,
                                   std::vector<AlphaUnit>& alpha_array,
                                   int& find_img_ind,
                                   float& find_img_alpha) {
  if (alpha_array.size() == 0) return false;
  // Since alpha_array has already been sorted, we use binary search to find
  // the best-match result.
  int finder = -1;
  int min_ind = 0;
  int mid_ind = -1;
  int max_ind = static_cast<int>(alpha_array.size()) - 1;
  while (min_ind + 1 < max_ind) {
    mid_ind = (min_ind + max_ind) / 2;
    if (alpha_array[mid_ind].alpha_ == expect_alpha) {
      finder = mid_ind;
      break;
    } else if (alpha_array[mid_ind].alpha_ > expect_alpha) {
      max_ind = mid_ind - 1;
    } else {
      min_ind = mid_ind + 1;
    }
  }
  if (finder == -1) {
    if (fabs(alpha_array[max_ind].alpha_ - expect_alpha) >
        fabs(alpha_array[min_ind].alpha_ - expect_alpha)) finder = min_ind;
    else finder = max_ind;
  }
  
  // Dispatch image to leaf node.
  find_img_alpha = alpha_array[finder].alpha_;
  find_img_ind = alpha_array[finder].image_ind_;
  // Remove the find result from alpha_array.
//  std::cout<< alpha_array[finder].image_ind_ << std::endl;
  alpha_array.erase(alpha_array.begin() + finder);
  return true;
}

// Find the best fit aspect ratio (two images) in the given array.
// find_split_type returns 'h' or 'v'.
// If it is 'h', the parent node is horizontally split, and 'v' for vertically
// split. After finding the two images, the corresponding AlphaUnits are
// removed, which means we have dispatched two images.
bool CollageAdvanced::FindTwoImages(float expect_alpha,
                                    std::vector<AlphaUnit>& alpha_array,
                                    char& find_split_type,
                                    int& find_img_ind_1,
                                    float& find_img_alpha_1,
                                    int& find_img_ind_2,
                                    float& find_img_alpha_2) {
  if ((alpha_array.size() == 0) || (alpha_array.size() == 1)) return false;
  // There are two situations:
  // [1]: parent node is vertival cut.
  int i = 0;
  int j = static_cast<int>(alpha_array.size()) - 1;
  int best_v_i = i;
  int best_v_j = j;
  float min_v_diff = fabs(alpha_array[best_v_i].alpha_ +
                         alpha_array[best_v_j].alpha_ -
                         expect_alpha);
  while (i < j) {
    if (alpha_array[i].alpha_ + alpha_array[j].alpha_ > expect_alpha) {
      float diff = fabs(alpha_array[i].alpha_ +
                       alpha_array[j].alpha_ -
                       expect_alpha);
      if (diff < min_v_diff) {
        min_v_diff = diff;
        best_v_i = i;
        best_v_j = j;
      }
      --j;
    } else if (alpha_array[i].alpha_ + alpha_array[j].alpha_ < expect_alpha) {
      float diff = fabs(alpha_array[i].alpha_ +
                       alpha_array[j].alpha_ -
                       expect_alpha);
      if (diff < min_v_diff) {
        min_v_diff = diff;
        best_v_i = i;
        best_v_j = j;
      }
      ++i;
    } else {
      best_v_i = i;
      best_v_j = j;
      min_v_diff = 0;
      break;
    }
  }
  // [2]: parent node is horizontal cut;
  float expect_alpha_recip = 1 / expect_alpha;
  i = 0;
  j = static_cast<int>(alpha_array.size()) - 1;
  int best_h_i = i;
  int best_h_j = j;
  float min_h_diff = fabs(alpha_array[best_h_i].alpha_recip_ +
                         alpha_array[best_h_j].alpha_recip_ -
                         expect_alpha_recip);
  while (i < j) {
    if (alpha_array[i].alpha_recip_ + alpha_array[j].alpha_recip_ >
        expect_alpha_recip) {
      float diff = fabs(alpha_array[i].alpha_recip_ +
                       alpha_array[j].alpha_recip_ -
                       expect_alpha_recip);
      if (diff < min_h_diff) {
        min_h_diff = diff;
        best_h_i = i;
        best_h_j = j;
      }
      ++i;
    } else if (alpha_array[i].alpha_recip_  + alpha_array[j].alpha_recip_ <
        expect_alpha_recip) {
      float diff = fabs(alpha_array[i].alpha_recip_ +
                        alpha_array[j].alpha_recip_ -
                        expect_alpha_recip);
      if (diff < min_h_diff) {
        min_h_diff = diff;
        best_h_i = i;
        best_h_j = j;
      }
      --j;
    } else {
      best_h_i = i;
      best_h_j = j;
      min_h_diff = 0;
      break;
    }
  }
  
  // Find the best-match from the above two situations.
  float real_alpha_v = alpha_array[best_v_i].alpha_ + alpha_array[best_v_j].alpha_;
  float real_alpha_h = (alpha_array[best_h_i].alpha_ * alpha_array[best_h_j].alpha_) /
  (alpha_array[best_h_i].alpha_ + alpha_array[best_h_j].alpha_);
  
  float ratio_diff_v = -1;
  float ratio_diff_h = -1;
  if (real_alpha_v > expect_alpha) {
    ratio_diff_v = real_alpha_v / expect_alpha;
  } else {
    ratio_diff_v = expect_alpha / real_alpha_v;
  }
  if (real_alpha_h > expect_alpha) {
    ratio_diff_h = real_alpha_h / expect_alpha;
  } else {
    ratio_diff_h = expect_alpha / real_alpha_h;
  }

  assert(best_h_i < best_h_j);
  assert(best_v_i < best_v_j);
  
  if (ratio_diff_v <= ratio_diff_h) {
    find_split_type = 'v';
    find_img_ind_1 = alpha_array[best_v_i].image_ind_;
    find_img_ind_2 = alpha_array[best_v_j].image_ind_;
    find_img_alpha_1 = alpha_array[best_v_i].alpha_;
    find_img_alpha_2 = alpha_array[best_v_j].alpha_;
    
//    std::cout << alpha_array[best_v_i].image_ind_
//    << ":" << alpha_array[best_v_j].image_ind_ << std::endl;
    
    alpha_array.erase(alpha_array.begin() + best_v_j);
    alpha_array.erase(alpha_array.begin() + best_v_i);
  } else {
    find_split_type = 'h';
    find_img_ind_1 = alpha_array[best_h_i].image_ind_;
    find_img_ind_2 = alpha_array[best_h_j].image_ind_;
    find_img_alpha_1 = alpha_array[best_h_i].alpha_;
    find_img_alpha_2 = alpha_array[best_h_j].alpha_;
    
//    std::cout << alpha_array[best_h_i].image_ind_
//    << ":" << alpha_array[best_h_j].image_ind_ << std::endl;
    
    alpha_array.erase(alpha_array.begin() + best_h_j);
    alpha_array.erase(alpha_array.begin() + best_h_i);
  }
  return true;
}

void CollageAdvanced::AdjustAlpha(TreeNode *node, float thresh) {
  assert(thresh > 1);
  if (node->is_leaf_) return;
  if (node == NULL) return;
  
  float thresh_2 = 1 + (thresh - 1) / 2;
  
  if (node->alpha_ > node->alpha_expect_ * thresh_2) {
    // Too big actual aspect ratio.
    node->split_type_ = 'h';
    node->left_child_->alpha_expect_ = node->alpha_expect_ * 2;
    node->right_child_->alpha_expect_ = node->alpha_expect_ * 2;
  } else if (node->alpha_ < node->alpha_expect_ / thresh_2 ) {
    // Too small actual aspect ratio.
    node->split_type_ = 'v';
    node->left_child_->alpha_expect_ = node->alpha_expect_ / 2;
    node->right_child_->alpha_expect_ = node->alpha_expect_ / 2;
  } else {
    // Aspect ratio is okay.
    if (node->split_type_ == 'h') {
      node->left_child_->alpha_expect_ = node->alpha_expect_ * 2;
      node->right_child_->alpha_expect_ = node->alpha_expect_ * 2;
    } else if (node->split_type_ == 'v') {
      node->left_child_->alpha_expect_ = node->alpha_expect_ * 2;
      node->right_child_->alpha_expect_ = node->alpha_expect_ * 2;
    } else {
      std::cout << "Error: AdjustAlpha" << std::endl;
      return;
    }
  }
  AdjustAlpha(node->left_child_, thresh);
  AdjustAlpha(node->right_child_, thresh);
}