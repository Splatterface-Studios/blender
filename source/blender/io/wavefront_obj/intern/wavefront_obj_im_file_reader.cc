/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2020 Blender Foundation.
 * All rights reserved.
 */

/** \file
 * \ingroup obj
 */

#include <fstream>
#include <iostream>

#include "BKE_context.h"

#include "BLI_string_ref.hh"
#include "BLI_vector.hh"

#include "wavefront_obj_ex_file_writer.hh"
#include "wavefront_obj_im_file_reader.hh"

namespace blender::io::obj {

using std::string;

OBJImporter::OBJImporter(const OBJImportParams &import_params) : import_params_(import_params)
{
  infile_.open(import_params_.filepath);
}

/**
 * Split the given string by the delimiter and fill the given vector.
 * If an intermediate string is empty, or space or null character, it is not appended to the
 * vector.
 */
static void split_by_char(const string &in_string, char delimiter, Vector<string> &r_out_list)
{
  std::stringstream stream(in_string);
  string word{};
  while (std::getline(stream, word, delimiter)) {
    if (word.empty() || word[0] == ' ' || word[0] == '\0') {
      continue;
    }
    r_out_list.append(word);
  }
}

/**
 * Substring of the given string from the start to the first ` ` if encountered.
 * If no space is found in the string, return the first character.
 */
static string first_word_of_string(const string &in_string)
{
  size_t pos = in_string.find_first_of(' ');
  return pos == string::npos ? in_string.substr(0, 1) : in_string.substr(0, pos);
}

/**
 * Convert all members of the Span of strings to floats and assign them to the float
 * array members. Usually used for values like coordinates.
 *
 * Catches exception if the string cannot be converted to a float. The float array members
 *  are set to <TODO ankitm: values can be -1.0 too!> in that case.
 */

BLI_INLINE void copy_string_to_float(Span<string> src, MutableSpan<float> r_dst)
{
  BLI_assert(src.size() == r_dst.size());
  for (int i = 0; i < r_dst.size(); ++i) {
    try {
      r_dst[i] = std::stof(src[i]);
    }
    catch (const std::invalid_argument &inv_arg) {
      fprintf(stderr, "Bad conversion to float:%s:%s\n", inv_arg.what(), src[i].c_str());
      r_dst[i] = -1.0f;
    }
  }
}

/**
 * Convert the given string to int and assign it to the destination value.
 *
 * Catches exception if the string cannot be converted to an integer. The destination
 *  int is set to <TODO ankitm: indices can be -1 too!> in that case.
 */
BLI_INLINE void copy_string_to_int(const string &src, int &r_dst)
{
  try {
    r_dst = std::stoi(src);
  }
  catch (const std::invalid_argument &inv_arg) {
    fprintf(stderr, "Bad conversion to int:%s:%s\n", inv_arg.what(), src.c_str());
    r_dst = -1;
  }
}

/**
 * Convert the given strings to ints and fill the destination int buffer.
 *
 * Catches exception if a string cannot be converted to an integer. The destination
 *  int is set to <TODO ankitm: indices can be -1 too!> in that case.
 */
BLI_INLINE void copy_string_to_int(Span<string> src, MutableSpan<int> r_dst)
{
  BLI_assert(src.size() == r_dst.size());
  for (int i = 0; i < r_dst.size(); ++i) {
    copy_string_to_int(src[i], r_dst[i]);
  }
}

/**
 * Based on the properties of the given raw object, return whether a new raw object
 * should be created. Caller should get some hint that the encountered object is a curve before
 * calling this function.
 *
 * This relies on the fact that the object type is updated to include CU_NURBS only _after_
 * this function returns true.
 */
static bool should_create_new_curve(std::unique_ptr<OBJRawObject> *raw_object)
{
  if (raw_object) {
    /* After the creation of a raw object, at least one element has been found in the OBJ file
     * that indicates that this is a mesh, not a curve. */
    if ((*raw_object)->face_elements.size() || (*raw_object)->uv_vertex_indices.size() ||
        (*raw_object)->tot_normals) {
      return true;
    }
    else {
      /* If not, then the given object could be a curve with all fields complete.
       * So create a new object if its type contains CU_NURBS. */
      return (*raw_object)->object_type & (OB_CURVE | CU_NURBS);
    }
  }
  return true;
}

void OBJImporter::parse_and_store(Vector<std::unique_ptr<OBJRawObject>> &list_of_objects,
                                  GlobalVertices &global_vertices)
{
  string line;
  /* Non owning raw pointer to the unique_ptr to a raw object.
   * Needed to update object data in the same while loop.
   * TODO ankitm Try to move the rest of the data parsing code in a conditional
   * depending on a valid "o" object. */
  std::unique_ptr<OBJRawObject> *curr_ob = nullptr;
  /* State-setting variable: if set, they remain the same for the remaining elements. */
  bool shaded_smooth = false;
  string object_group{};

  while (std::getline(infile_, line)) {
    string line_key = first_word_of_string(line);
    std::stringstream s_line(line.substr(line_key.size()));

    if (line_key == "o") {
      /* Update index offsets if an object has been processed already. */
      if (curr_ob) {
        index_offsets[VERTEX_OFF] += (*curr_ob)->vertex_indices.size();
        index_offsets[UV_VERTEX_OFF] += (*curr_ob)->uv_vertex_indices.size();
      }
      list_of_objects.append(std::make_unique<OBJRawObject>(s_line.str()));
      curr_ob = &list_of_objects.last();
      (*curr_ob)->object_type = OB_MESH;
    }
    /* TODO ankitm Check that an object exists. */
    else if (line_key == "v") {
      float3 curr_vert;
      Vector<string> str_vert_split;
      split_by_char(s_line.str(), ' ', str_vert_split);
      copy_string_to_float(str_vert_split, {curr_vert, 3});
      global_vertices.vertices.append(curr_vert);
      if (curr_ob) {
        (*curr_ob)->vertex_indices.append(global_vertices.vertices.size() - 1);
      }
    }
    else if (line_key == "vn") {
      (*curr_ob)->tot_normals++;
    }
    else if (line_key == "vt") {
      float2 curr_uv_vert;
      Vector<string> str_uv_vert_split;
      split_by_char(s_line.str(), ' ', str_uv_vert_split);
      copy_string_to_float(str_uv_vert_split, {curr_uv_vert, 2});
      global_vertices.uv_vertices.append(curr_uv_vert);
      if (curr_ob) {
        (*curr_ob)->uv_vertex_indices.append(global_vertices.uv_vertices.size() - 1);
      }
    }
    else if (line_key == "l") {
      int edge_v1, edge_v2;
      Vector<string> str_edge_split;
      split_by_char(s_line.str(), ' ', str_edge_split);
      copy_string_to_int(str_edge_split[0], edge_v1);
      copy_string_to_int(str_edge_split[1], edge_v2);
      /* Remove the indices of vertices "claimed" by other raw objects. "+ 1" is to make the OBJ
       * indices one-based to C++ zero-based. In the other case, make relative index like -1 to
       * point to the last vertex recorded in the memory. */
      edge_v1 -= edge_v1 > 0 ? index_offsets[VERTEX_OFF] + 1 : -(global_vertices.vertices.size());
      edge_v2 -= edge_v2 > 0 ? index_offsets[VERTEX_OFF] + 1 : -(global_vertices.vertices.size());
      BLI_assert(edge_v1 > 0 && edge_v2 > 0);
      (*curr_ob)->edges.append({static_cast<uint>(edge_v1), static_cast<uint>(edge_v2)});
    }
    else if (line_key == "g") {
      object_group = s_line.str();
      if (object_group.find("off") != string::npos || object_group.find("null") != string::npos) {
        object_group = {};
      }
    }
    else if (line_key == "s") {
      string str_shading;
      s_line >> str_shading;
      if (str_shading != "0" && str_shading.find("off") == string::npos &&
          str_shading.find("null") == string::npos) {
        /* TODO ankitm make a string to bool function if need arises. */
        try {
          std::stoi(str_shading);
          shaded_smooth = true;
        }
        catch (const std::invalid_argument &inv_arg) {
          fprintf(stderr,
                  "Bad argument for smooth shading: %s:%s\n",
                  inv_arg.what(),
                  str_shading.c_str());
          shaded_smooth = false;
        }
      }
      else {
        shaded_smooth = false;
      }
    }
    else if (line_key == "f") {
      OBJFaceElem curr_face;
      curr_face.shaded_smooth = shaded_smooth;

      Vector<string> str_corners_split;
      split_by_char(s_line.str(), ' ', str_corners_split);
      for (auto &str_corner : str_corners_split) {
        OBJFaceCorner corner;
        size_t n_slash = std::count(str_corner.begin(), str_corner.end(), '/');
        if (n_slash == 0) {
          /* Case: f v1 v2 v3 . */
          copy_string_to_int({str_corner}, corner.vert_index);
        }
        else if (n_slash == 1) {
          /* Case: f v1/vt1 v2/vt2 v3/vt3 . */
          Vector<string> vert_uv_split;
          split_by_char(str_corner, '/', vert_uv_split);
          copy_string_to_int(vert_uv_split[0], corner.vert_index);
          if (vert_uv_split.size() == 2) {
            copy_string_to_int(vert_uv_split[1], corner.uv_vert_index);
            (*curr_ob)->tot_uv_verts++;
          }
        }
        else if (n_slash == 2) {
          /* Case: f v1//vn1 v2//vn2 v3//vn3 . */
          /* Case: f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 . */
          Vector<string> vert_uv_normal_split;
          split_by_char(str_corner, '/', vert_uv_normal_split);
          copy_string_to_int(vert_uv_normal_split[0], corner.vert_index);
          if (vert_uv_normal_split.size() == 3) {
            copy_string_to_int(vert_uv_normal_split[1], corner.uv_vert_index);
            (*curr_ob)->tot_uv_verts++;
          }
          /* Discard normals. They'll be calculated on the basis of smooth
           * shading flag. */
        }
        corner.vert_index += corner.vert_index < 0 ? index_offsets[VERTEX_OFF] + 1 :
                                                     -(index_offsets[VERTEX_OFF] + 1);
        corner.uv_vert_index += corner.uv_vert_index < 0 ? index_offsets[UV_VERTEX_OFF] + 1 :
                                                           -(index_offsets[UV_VERTEX_OFF] + 1);

        curr_face.face_corners.append(corner);
      }

      (*curr_ob)->face_elements.append(curr_face);
      (*curr_ob)->tot_loop += curr_face.face_corners.size();
    }
    else if (line_key == "cstype") {
      if (s_line.str().find("bspline") != string::npos) {
        if (should_create_new_curve(curr_ob)) {
          list_of_objects.append(std::make_unique<OBJRawObject>("NURBSCurve"));
          curr_ob = &list_of_objects.last();
          (*curr_ob)->nurbs_element.group = object_group;
          /* Make sure that the flags are overridden & only after a new object is created. */
          (*curr_ob)->object_type = OB_CURVE | CU_NURBS;
        }
      }
      else {
        fprintf(stderr, "Type:'%s' not supported\n", s_line.str().c_str());
      }
    }
    else if (line_key == "deg") {
      copy_string_to_int({s_line.str()}, (*curr_ob)->nurbs_element.degree);
    }
    else if (line_key == "curv") {
      Vector<string> str_curv_split;
      split_by_char(s_line.str(), ' ', str_curv_split);
      /* Remove "0.0" and "1.0" from the strings. They are hardcoded. */
      str_curv_split.remove(0);
      str_curv_split.remove(0);
      (*curr_ob)->nurbs_element.curv_indices.resize(str_curv_split.size());
      copy_string_to_int(str_curv_split, (*curr_ob)->nurbs_element.curv_indices);
      for (auto &curv_point : (*curr_ob)->nurbs_element.curv_indices) {
        curv_point -= curv_point > 0 ? 1 : -(global_vertices.vertices.size());
      }
    }
    else if (line_key == "parm") {
      Vector<string> str_parm_split;
      split_by_char(s_line.str(), ' ', str_parm_split);
      if (str_parm_split[0] == "u" || str_parm_split[0] == "v") {
        str_parm_split.remove(0);
        (*curr_ob)->nurbs_element.parm.resize(str_parm_split.size());
        copy_string_to_float(str_parm_split, (*curr_ob)->nurbs_element.parm);
      }
      else {
        fprintf(stderr, "Surfaces not supported: %s\n", str_parm_split[0].c_str());
      }
    }
    else if (line_key == "end") {
      object_group = {};
    }
    else if (line_key == "usemtl") {
      (*curr_ob)->material_name.append(s_line.str());
    }
    else if (line_key == "#") {
    }
  }
}
}  // namespace blender::io::obj
