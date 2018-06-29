/*------------------- 
** Poisson object (prototype)
** Author: Xia Sun
--------------------*/
#ifndef __MESH_H__
#define __MESH_H__

#ifndef WITH_OPENCV
#define WITH_OPENCV

#include <slam6d/scan.h>
#include <slam6d/normals.h>

#ifdef WITH_OPENCV
#include <normals/normals_panorama.h>
#endif

class Poisson {
public:
  // public attributes
  
  
  // public methods
  Poisson();
  ~Poisson();
  int setVertices(vector<Point> v);
  int setNormals(vector<Point> n);
  int setParams(PoissonParam &p);
  int getMesh(CoredVectorMeshData *m);
  int apply();
  int testVcgFilter(); // test mesh processing with vcglib
  int calcNormalVcg();
  int exportMesh(const char *modelPath);

private:
  // private attributes
  int reconstructed;
  PoissonParam params;
  vector<Point3D<float>> vertices;
  vector<Point3D<float>> normals;
  CoredVectorMeshData *mesh;
  Point3D<float> center;
  float scale;
  vcg::CallBackPos *cb;

  // private methods
  void initialize();
  int ready();
};

#endif
#endif