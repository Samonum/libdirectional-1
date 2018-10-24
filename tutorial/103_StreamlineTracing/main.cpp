#include <igl/barycenter.h>
#include <igl/edge_topology.h>
#include <igl/local_basis.h>
#include <igl/parula.h>
#include <igl/per_face_normals.h>
#include <igl/per_vertex_normals.h>
#include <igl/read_triangle_mesh.h>
#include <igl/readOFF.h>
#include <igl/slice.h>
#include <igl/sort_vectors_ccw.h>
#include <directional/streamlines.h>
//#include <igl/copyleft/comiso/nrosy.h>
#include <igl/opengl/glfw/Viewer.h>
#include <directional/power_field.h>
#include <directional/power_to_raw.h>
#include <directional/line_cylinders.h>

#include <cstdlib>
#include <iostream>
#include <vector>
#include <fstream>


// Mesh
Eigen::MatrixXd VMesh, VField, CField, CMesh;
Eigen::MatrixXi FMesh, FField;

Eigen::VectorXi cIDs;
Eigen::MatrixXd cValues;
Eigen::MatrixXcd powerField;
Eigen::MatrixXd raw;

directional::StreamlineData sl_data;
directional::StreamlineState sl_state;

int degree;         // degree of the vector field
int half_degree;    // degree/2 if treat_as_symmetric
bool treat_as_symmetric = true;

int anim_t = 0;
int anim_t_dir = 1;


bool pre_draw(igl::opengl::glfw::Viewer &viewer)
{
  using namespace Eigen;
  using namespace std;
  
  if (!viewer.core.is_animating)
    return false;
  
  directional::streamlines_next(VMesh, FMesh, sl_data, sl_state);
  Eigen::RowVector3d color = Eigen::RowVector3d::Zero();
  double value = ((anim_t) % 100) / 100.;
  
  if (value > 0.5)
    value = 1 - value;
  value = value / 0.5;
  igl::parula(value, color[0], color[1], color[2]);
  
  //CField=color.replicate(FField.rows(),1);
  
  Eigen::MatrixXd VFieldNew, CFieldNew;
  Eigen::MatrixXi FFieldNew;
  directional::line_cylinders(sl_state.start_point, sl_state.end_point, 0.0005, color.replicate(sl_state.start_point.rows(),1) /*Eigen::MatrixXd::Constant(sl_state.start_point.rows(),3,1.0)*/, 4, VFieldNew, FFieldNew, CFieldNew);
  
  //extending current streamline mesh
  FField.conservativeResize(FField.rows()+FFieldNew.rows(),3);
  FField.block(FField.rows()-FFieldNew.rows(),0,FFieldNew.rows(),3)=FFieldNew.array()+VField.rows();
  
  VField.conservativeResize(VField.rows()+VFieldNew.rows(),3);
  VField.block(VField.rows()-VFieldNew.rows(),0,VFieldNew.rows(),3) = VFieldNew;
  
  CField.conservativeResize(CField.rows()+CFieldNew.rows(),3);
  CField.block(CField.rows()-CFieldNew.rows(),0,CFieldNew.rows(),3) = CFieldNew;
  
  
  viewer.selected_data_index=1;  //streamline mesh
  viewer.data().clear();
  viewer.data().set_mesh(VField, FField);
  viewer.data().set_colors(CField);
  anim_t += anim_t_dir;
  
  return false;
}

bool key_down(igl::opengl::glfw::Viewer &viewer, unsigned char key, int modifier)
{
  if (key == ' ')
  {
    viewer.core.is_animating = !viewer.core.is_animating;
    return true;
  }
  return false;
}

int main(int argc, char *argv[])
{
  using namespace Eigen;
  using namespace std;
  
  igl::opengl::glfw::Viewer viewer;
  
  // Load a mesh in OFF format
  igl::readOFF(TUTORIAL_SHARED_PATH "/lion.off", VMesh, FMesh);
  // Create a Vector Field
  Eigen::VectorXi b;
  Eigen::MatrixXd bc;
  Eigen::VectorXd S; // unused
  
  b.resize(1);
  b << 0;
  bc.resize(1, 3);
  bc << 1, 1, 1;
  

  half_degree = 3;
  treat_as_symmetric = true;
  
  directional::power_field(VMesh, FMesh, b,  bc , 4, powerField);
  
  // Convert it to raw field
  directional::power_to_raw(VMesh,FMesh,powerField,4,raw, true);
  
  directional::streamlines_init(VMesh, FMesh, raw, sl_data, sl_state);
  
  //triangle mesh
  viewer.data().set_mesh(VMesh, FMesh);
  CMesh.setConstant(FMesh.rows(), 3, 0.1);
  viewer.data().set_colors(CMesh);
  viewer.data().show_lines = false;
  
  // Viewer Settings
  viewer.callback_pre_draw = &pre_draw;
  viewer.callback_key_down = &key_down;
  viewer.core.is_animating = false;
  viewer.core.animation_max_fps = 30.;
  
  // Draw initial seeds on sample points
  directional::StreamlineState sl_state0;
  sl_state0 = sl_state;
  directional::streamlines_next(VMesh, FMesh, sl_data, sl_state0);
  Eigen::MatrixXd v = sl_state0.end_point - sl_state0.start_point;
  v.rowwise().normalize();
  
  //streamline mesh
  viewer.append_mesh();
  directional::line_cylinders(sl_state0.start_point, sl_state0.start_point + 0.0005 * v, 0.0005, Eigen::MatrixXd::Constant(sl_state0.start_point.rows(),3,1.0), 4, VField, FField, CField);
  viewer.data().set_mesh(VField, FField);
  viewer.data().set_colors(CField);
  viewer.data().show_lines = false;
  cout <<
  "Press [space] to toggle animation" << endl;
  viewer.launch();
}
