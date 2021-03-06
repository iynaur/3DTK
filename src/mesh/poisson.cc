#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "mesh/poisson.h"
#include "poisson/PoissonRecon.h"
#include "poisson/SurfaceTrimmer.h"

using namespace std;

Poisson::Poisson() {
  reset();
}

Poisson::~Poisson() {
  reset();
}

// called by (de)constructor or used for reset
void Poisson::reset() {
  reconstructed = 0;
  updated = 0;
  trimmed = 0;
  // reset points, normals and colors only when they are not setted yet
  if (points.size() != 0 && normals.size() != 0 && colors.size() != 0) {
    for (int i = 0; i < points.size(); ++i) {
      delete [] points[i];
    }
    for (int i = 0; i < normals.size(); ++i) {
      delete [] normals[i];
    }
    for (int i = 0; i < colors.size(); ++i) {
      delete [] colors[i];
    }
    points.clear();
    normals.clear();
    colors.clear();
  }
  for (int i = 0; i < vertices.size(); ++i) {
    delete [] vertices[i];
  }
  vertices.clear();
  for (int i = 0; i < faces.size(); ++i) {
    delete [] faces[i];
  }
  faces.clear();
  for (int i = 0; i < tVertices.size(); ++i) {
    delete [] tVertices[i];
  }
  tVertices.clear();
  for (int i = 0; i < tFaces.size(); ++i) {
    delete [] tFaces[i];
  }
  tFaces.clear();
  return;
}

int Poisson::setPoints(vector<vector<float>> &v) {
  reset();
  points.resize(v.size());
  for (int i = 0; i < points.size(); ++i) {
    points[i] = new float[3];
    points[i][0] = v[i][0]; 
    points[i][1] = v[i][1]; 
    points[i][2] = v[i][2];
  }
  return 1;
}

int Poisson::setNormals(vector<vector<float>> &n) {
  reset();
  normals.resize(n.size());
  for (int i = 0; i < normals.size(); ++i) {
    normals[i] = new float[3];
    normals[i][0] = n[i][0]; 
    normals[i][1] = n[i][1]; 
    normals[i][2] = n[i][2];
  }
  return 1;
}

int Poisson::setColors(std::vector<std::vector<float>> &c) {
  reset();
  colors.resize(c.size());
  for (int i = 0; i < colors.size(); ++i) {
    colors[i] = new float[3];
    colors[i][0] = c[i][0]; 
    colors[i][1] = c[i][1]; 
    colors[i][2] = c[i][2];
  }
  return 1;
}

int Poisson::setParams(PoissonParam &p) {
  Depth.set = true; Depth.value = p.Depth;
  Density.set = true;
  Colors.set = true;
  Normals.set = true;
  Out.set = true;
  Trim.set = true; Trim.value = p.Trim;
  SamplesPerNode.set = true; SamplesPerNode.value = p.samplesPerNode;
  if( !PointWeight.set ) PointWeight.value = DefaultPointWeightMultiplier*Degree.value;
  return 1;
}

// export originally generated mesh by poisson
int Poisson::exportMesh(const char* modelPath) {
  if (!updated) {
    cout << "Fail to export mesh due to reconstruction failure" << endl;
    return 0;
  }
  fstream fs(modelPath, fstream::out);
  // get and write correct scaled points coordinates
  for (int i = 0; i < vertices.size(); ++i) {
    fs << "v " << vertices[i][0] << " " 
      << vertices[i][1] << " " 
      << vertices[i][2] << " "
      << vertices[i][7] << " "
      << vertices[i][8] << " "
      << vertices[i][9] << " " << endl;
  }
  // get and write vertices normals
  for (int i = 0; i < vertices.size() && params.ExportNormal; ++i) {
    fs << "vn " << vertices[i][4] << " " 
      << vertices[i][5] << " " 
      << vertices[i][6] << " " << endl;
  }
  // get and write correct ordered faces indexes
  for (int i = 0; i < faces.size(); ++i) {
    fs << "f " << faces[i][0] + 1 << " " 
      << faces[i][1] + 1 << " " 
      << faces[i][2] + 1 << " " << endl;
  }
  fs.close();
  return 1;
}

// export trimmed mesh generated by poisson
int Poisson::exportTrimmedMesh(const char* modelPath) {
  if (!trimmed) {
    cout << "Fail to export mesh due to trimming failure" << endl;
    return 0;
  }
  fstream fs(modelPath, fstream::out);
  // get and write correct scaled points coordinates
  for (int i = 0; i < tVertices.size(); ++i) {
    fs << "v " << tVertices[i][0] << " " 
      << tVertices[i][1] << " " 
      << tVertices[i][2] << " "
      << tVertices[i][7] << " "
      << tVertices[i][8] << " "
      << tVertices[i][9] << " " << endl;
  }
  // get and write vertices normals
  for (int i = 0; i < tVertices.size() && params.ExportNormal; ++i) {
    fs << "vn " << tVertices[i][4] << " " 
      << tVertices[i][5] << " " 
      << tVertices[i][6] << " " << endl;
  }
  // get and write correct ordered faces indexes
  for (int i = 0; i < tFaces.size(); ++i) {
    fs << "f " << tFaces[i][0] + 1 << " " 
      << tFaces[i][1] + 1 << " " 
      << tFaces[i][2] + 1 << " " << endl;
  }
  fs.close();
  return 1;
}

// filter mesh with density value
// implementation in SurfaceTrimmer.h
int Poisson::surfaceTrimmer(float dstVal) {
  // trimmed = CallSurfaceTrimmer(dstVal, vertices, faces, tVertices, tFaces);
  // return trimmed;
  return 0;
}

int Poisson::apply() {
  if (points.size() == 0 || normals.size() == 0 || points.size() != normals.size()) {
    cout << "Fail to call poisson with wrong points and normals" << endl;
    return 0;
  }
  typedef MultiPointStreamData< float , PointStreamColor< float > > AdditionalPointSampleData;
  typedef PlyVertexWithData< float , 3 , MultiPointStreamData< float , PointStreamNormal< float , 3 > , PointStreamValue< float > , AdditionalPointSampleData > > Vertex;
  typedef PlyVertexWithData< float , 3 , MultiPointStreamData< float , PointStreamValue< float > , PointStreamNormal< float , 3 > , PointStreamColor< float > > > Vertex2;

  XForm< float , 4 > xForm;
  CoredFileMeshData< Vertex > mesh( " " );

  // original vertices and polygon of mesh, used for surface trimmer
  std::vector< Vertex2 > oVertices;
	std::vector< std::vector< int > > oPolygons, tPolygons;

  Execute< float, CoredFileMeshData< Vertex >, PointStreamColor< float > >(mesh, xForm, points, normals, colors, IsotropicUIntPack< 3 , FEMDegreeAndBType< 1 , BOUNDARY_NEUMANN >::Signature >());
  // update vertices and faces
  {
    int i = 0;
    int nr_vertices=int(mesh.outOfCorePointCount()+mesh.inCorePoints.size());
    int nr_faces=mesh.polygonCount();

    mesh.resetIterator();
    typename Vertex::Transform _xForm( xForm );

    // update vertices info including position, density, color and normal
    for( i=0 ; i<int( mesh.inCorePoints.size() ) ; i++ )
    {
      Vertex vertex = _xForm( mesh.inCorePoints[i] );

      // construct vertex2
      Vertex2 vertex2;
      vertex2.point = vertex.point;
      std::get<0>(vertex2.data.data) = std::get<1>(vertex.data.data);
      std::get<1>(vertex2.data.data) = std::get<0>(vertex.data.data);
      std::get<2>(vertex2.data.data) = std::get<0>(std::get<2>(vertex.data.data).data);
      oVertices.push_back(vertex2);

      auto vert = vertex.point;
      auto norm = std::get<0>(vertex.data.data).data;
      auto density = std::get<1>(vertex.data.data).data;
      auto color = std::get<0>(std::get<2>(vertex.data.data).data).data;
      float *v = new float[10];
      v[0] = vert.coords[0]; // xyz
      v[1] = vert.coords[1];
      v[2] = vert.coords[2];
      v[3] = std::get<1>(vertex.data.data).data; // density
      v[4] = norm.coords[0]; // nx ny nz
      v[5] = norm.coords[1];
      v[6] = norm.coords[2];
      v[7] = color.coords[0]; // rgb
      v[8] = color.coords[1];
      v[9] = color.coords[2];
      vertices.push_back(v);
    }
    for( i=0; i<mesh.outOfCorePointCount() ; i++ )
    {
      Vertex vertex;
      mesh.nextOutOfCorePoint( vertex );
      // vertex = _xForm( vertex );

      // construct vertex2
      Vertex2 vertex2;
      vertex2.point = vertex.point;
      std::get<0>(vertex2.data.data) = std::get<1>(vertex.data.data);
      std::get<1>(vertex2.data.data) = std::get<0>(vertex.data.data);
      std::get<2>(vertex2.data.data) = std::get<0>(std::get<2>(vertex.data.data).data);
      oVertices.push_back(vertex2);

      auto vert = vertex.point;
      auto norm = std::get<0>(vertex.data.data).data;
      auto density = std::get<1>(vertex.data.data).data;
      auto color = std::get<0>(std::get<2>(vertex.data.data).data).data;
      float *v = new float[10];
      v[0] = vert.coords[0]; // xyz
      v[1] = vert.coords[1];
      v[2] = vert.coords[2];
      v[3] = std::get<1>(vertex.data.data).data; // density
      v[4] = norm.coords[0]; // nx ny nz
      v[5] = norm.coords[1];
      v[6] = norm.coords[2];
      v[7] = color.coords[0]; // rgb
      v[8] = color.coords[1];
      v[9] = color.coords[2];
      vertices.push_back(v);
    }

    // update faces info
    std::vector< CoredVertexIndex > polygon;
    for( i=0 ; i<nr_faces ; i++ )
    {
      //
      // create and fill a struct that the ply code can handle
      //
      PlyFace ply_face;
      mesh.nextPolygon( polygon );
      ply_face.nr_vertices = int( polygon.size() );
      ply_face.vertices = new int[ polygon.size() ];
      int *f = new int[3];
      for( int i=0 ; i<int(polygon.size()) ; i++ ) {
        if( polygon[i].inCore ) ply_face.vertices[i] = polygon[i].idx;
        else                    ply_face.vertices[i] = polygon[i].idx + int( mesh.inCorePoints.size() );
        f[i] = ply_face.vertices[i];
      }
      faces.push_back(f);
      oPolygons.push_back({f[0], f[1], f[2]});
      delete[] ply_face.vertices;
    }

    updated = 1;
  }

  // call surface trimmer
  int a = oVertices.size(), b = oPolygons.size();
  Execute< PointStreamNormal< float , DIMENSION > , PointStreamColor< float > >(oVertices, oPolygons, tPolygons);

  // update trimmed vertices and polygons
  {
    for (int i = 0; i < oVertices.size(); ++i) {
      auto vert = oVertices[i].point;
      auto norm = std::get<1>(oVertices[i].data.data).data;
      auto density = std::get<0>(oVertices[i].data.data).data;
      auto color = std::get<2>(oVertices[i].data.data).data;
      float *v = new float[10];
      v[0] = vert.coords[0]; // xyz
      v[1] = vert.coords[1];
      v[2] = vert.coords[2];
      v[3] = density; // density
      v[4] = norm.coords[0]; // nx ny nz
      v[5] = norm.coords[1];
      v[6] = norm.coords[2];
      v[7] = color.coords[0]; // rgb
      v[8] = color.coords[1];
      v[9] = color.coords[2];
      tVertices.push_back(v);
    }
    for (int i = 0; i < tPolygons.size(); ++i) {
      int *f = new int[3];
      f[0] = tPolygons[i][0]; f[1] = tPolygons[i][1]; f[2] = tPolygons[i][2];
      tFaces.push_back(f);
    }
  }
  trimmed = 1;

  return reconstructed && updated;
}
