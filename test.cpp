/*
Copyright 2015, Giacomo Dabisias"
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
@Author 
Giacomo  Dabisias, PhD Student
PERCRO, (Laboratory of Perceptual Robotics)
Scuola Superiore Sant'Anna
via Luigi Alamanni 13D, San Giuliano Terme 56010 (PI), Italy

Modificado por:
Carlos Francisco Villaseņor Altamirano
CIDETEC IPN Mexico CD.MX.
*/
#include "k2g.h"
#include <iostream>
#include <ctime> 
#include <pcl/visualization/cloud_viewer.h>
#include <pcl/segmentation/segment_differences.h>
#include <pcl/console/print.h>
#include <pcl/console/parse.h>
#include <pcl/console/time.h>
#include <pcl/io/ply_io.h>
#include <pcl/kdtree/kdtree.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl/sample_consensus/ransac.h>
#include <pcl/sample_consensus/sac_model_plane.h>
#include <pcl/sample_consensus/sac_model_sphere.h>
#include <pcl/sample_consensus/sac_model_cylinder.h>
#include <pcl/features/normal_3d.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/extract_indices.h>

typedef pcl::PointXYZRGB PointType;
typedef pcl::Normal PointNType;



double tress = 0.01;
double down = 0.025;
bool _downSample = true;
bool _resta = true;
bool _cluster = false;
bool _ciclo = true;
bool _ciclo2 = false;


boost::shared_ptr<pcl::PointCloud<PointType>> cloudKinect;
pcl::PointCloud<PointType>::Ptr cloudAux(new pcl::PointCloud<PointType>);
pcl::PointCloud<PointType>::Ptr cloudAux2(new pcl::PointCloud<PointType>);
pcl::PointCloud<PointType>::Ptr cloudCopy(new pcl::PointCloud<PointType>);
pcl::PointCloud<PointType>::Ptr cloudOut(new pcl::PointCloud<PointType>);
pcl::NormalEstimation<PointType, PointNType > ne;
// Creating the KdTree object for the search method of the extraction
pcl::search::KdTree<PointType>::Ptr tree(new pcl::search::KdTree<PointType>);
std::vector<pcl::PointIndices> cluster_indices;


struct PlySaver{


  PlySaver(boost::shared_ptr<pcl::PointCloud<PointType>> cloud, bool binary, bool use_camera, K2G & k2g):
           cloud_(cloud), binary_(binary), use_camera_(use_camera), k2g_(k2g){}

  boost::shared_ptr<pcl::PointCloud<PointType>> cloud_;
  bool binary_;
  bool use_camera_;
  K2G & k2g_;

};

//entrada por teclado
void KeyboardEventOccurred(const pcl::visualization::KeyboardEvent &event, void * data)
{
	
  std::string pressed = event.getKeySym();//lee los datos de la interrupcion      
  PlySaver * s = (PlySaver*)data;//para cuardar la nube de puntos
  if(event.keyDown ())
  {
    if(pressed == "z")
    {
      
      pcl::PLYWriter writer;
      std::chrono::high_resolution_clock::time_point p = std::chrono::high_resolution_clock::now();
      std::string now = std::to_string((long)std::chrono::duration_cast<std::chrono::milliseconds>(p.time_since_epoch()).count());
      writer.write ("cloud_" + now, *(s->cloud_), s->binary_, s->use_camera_);
      
      std::cout << "saved " << "cloud_" + now + ".ply" << std::endl;
    }
	if(pressed == "y")
	{
		//se guarda una imagen inicial en cloud2 para luego compararla
		pcl::copyPointCloud(*cloudAux2, *cloudCopy);
		tree->setInputCloud(cloudOut);
		_cluster = true;
		std::cout << "copy" << std::endl;
	}
	if (pressed == "i")
	{
		//sirbe para debugear y ver que ocurre en cada ciclo
		//hay que descomentar una linea en main que hace _ciclo = false
		if (_ciclo)
			_ciclo = false;
		else
			_ciclo = true;

		std::cout << "pause" << std::endl;
	}
	if (pressed == "k")
	{
		//sirbe para debugear y ver que ocurre en cada ciclo
		//hay que descomentar una linea en main que hace _ciclo = false
		_ciclo = true;
		_ciclo2 = true;

		std::cout << "spin" << std::endl;
	}
	if (pressed == "x")
	{
		if (_resta)
			_resta = false;
		else
			_resta = true;
		std::cout << "substract " << _resta << std::endl;
	}
	if (pressed == "d")
	{
		if (_downSample)
			_downSample = false;
		else
			_downSample = true;
		std::cout << "downSample " << _downSample << std::endl;
	}
  }
}



int main(int argc, char * argv[])
{
	unsigned t0, t1;

	//se crea una ventana y se vincula con el objeto viewer 
	boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer(new pcl::visualization::PCLVisualizer("Input Data from Kinect"));
	boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer2(new pcl::visualization::PCLVisualizer("Output figures"));

	//se divide la ventana en 2 para mostrar la entrada y la salida
	//ventana 1
	//para el lado izq
	int v1(0);
	viewer->setPosition(350, 50);
	viewer->createViewPort(0.0, 0.0, 0.5, 1.0, v1);
	viewer->setBackgroundColor(0, 0, 0, v1);
	viewer->addText("Kinect data input", 10, 10, "v1 text", v1);
	viewer->addPointCloud<PointType>(cloudAux, "sample cloud", v1);
	viewer->setCameraPosition(-0.25, 0.0, -4.25, -0.025, -0.9999, 0.0, v1);
	//para el lado der
	int v2(0);
	viewer->createViewPort(0.5, 0.0, 1.0, 1.0, v2);
	viewer->setBackgroundColor(0.1, 0.1, 0.1, v2);
	viewer->addText("Filtered data", 10, 10, "v2 text", v2);
	viewer->addPointCloud<PointType>(cloudOut, "sample cloud2", v2);
	viewer->setCameraPosition(-0.25, 0.0, -4.25, -0.025, -0.9999, 0.0,v2);

	//ventana 2
	viewer2->setBackgroundColor(0, 0, 0);
	viewer2->setPosition(1240, 60);
	viewer2->setCameraPosition(-0.25, 0.0, -4.25, -0.025, -0.9999, 0.0);
	viewer2->addText("z: save cloud to ply", 10, 180, "v1 text2", v1);
	viewer2->addText("y: copy to filter", 10, 170, "v1 text3", v1);
	viewer2->addText("x: on/off filter", 10, 160, "v1 text4", v1);
	viewer2->addText("d: on/off douwn sample", 10, 150, "v1 text5", v1);
	viewer2->addText("i: pause the program", 10, 140, "v1 text6", v1);
	viewer2->addText("k: execute onece the algorithm", 10, 130, "v1 text7", v1);


	//se define los objetos para la captura de datos de kinect usando freenect y k2g
	Processor freenectprocessor = CUDA;//si no tienes cuda instalado prueba con OPENGL o CPU
	std::vector<int> ply_file_indices;
	K2G k2g(freenectprocessor);
	k2g.disableLog();



	//se agrega un evento para el teclado para poder guardar la nuve
	PlySaver ps(cloudAux2, false, false, k2g);
	viewer->registerKeyboardCallback(KeyboardEventOccurred, (void*)&ps);
	viewer2->registerKeyboardCallback(KeyboardEventOccurred, (void*)&ps);

	// para usar menos puntos de la nuve
	pcl::VoxelGrid<PointType> vg;
	vg.setInputCloud(cloudAux);
	vg.setLeafSize(down, down, down);

	// para realizar la dferencia entre 2 nuves de puntos
	//para eliminar los puntos que no se usan (filtro kalman)
	pcl::SegmentDifferences<PointType> resta;
	resta.setInputCloud(cloudAux2);
	resta.setTargetCloud(cloudCopy);
	resta.setDistanceThreshold(tress);

	//clusterizacion de objetos usando la distancia euclidiana
	pcl::EuclideanClusterExtraction<PointType> ec;
	ec.setClusterTolerance(down + 0.01); //la toleracia para el cluster es poco mas grande que la de la reduccion 
	ec.setMinClusterSize(50);
	ec.setMaxClusterSize(25000);
	ec.setSearchMethod(tree);
	ec.setInputCloud(cloudOut);
	ec.extract(cluster_indices);


	std::cout << "getting cloud" << std::endl;
	cloudKinect = k2g.getCloud();

	//inicia el ciclo con los datos ya iniciados
	while(!viewer->wasStopped() && !viewer2->wasStopped()){
		
		//permite la visualozacion de los datos en las ventanas
		viewer->spinOnce(1000);
		viewer2->spinOnce(1000); 

		//pausa la ejecucion
		if (_ciclo)
		{
			t0 = clock();

			//se actualizan los datos del kinect
			k2g.updateCloud(cloudKinect);
			//se copian a una nube auxiliar
			copyPointCloud(*cloudKinect, *cloudAux);

			if (_downSample)
			{
				//se reducen la cantidad de puntos
				vg.filter(*cloudAux2);
			}
			else
			{
				copyPointCloud(*cloudAux, *cloudAux2);
			}

			if (_resta)
			{
				//se eliminan los puntos que no se usan
				resta.segment(*cloudOut);
			}
			else
			{
				copyPointCloud(*cloudAux2, *cloudOut);
			}

			if (_cluster && _resta)
			{
				double probSphere = 0;
				double probPlane = 0;
				double probCylinder = 0;

				bool _sphere = false;
				bool _plane = false;
				bool _cylinder = false;

				viewer2->removeAllPointClouds();
				viewer2->removeAllShapes();
				//viewer2->addCoordinateSystem(1);

				//se obtienen los fiferentes objetos en diferentes nuves de puntos
				int j = 0;//contador de clusters
				std::cout << "Custering" << std::endl;
				std::vector<pcl::PointIndices> cluster_indices;//hay que reiniciarlos indices
				ec.extract(cluster_indices);
				//para cada nube de puntos(objetos) encontrada:
				for (std::vector<pcl::PointIndices>::const_iterator it = cluster_indices.begin(); it != cluster_indices.end(); ++it)
				{
					pcl::PointCloud<PointType>::Ptr cloud_cluster(new pcl::PointCloud<PointType>);
					pcl::PointCloud<PointType>::Ptr cloud_cluster2(new pcl::PointCloud<PointType>);
					pcl::PointCloud<PointNType>::Ptr cloud_normals(new pcl::PointCloud<PointNType>);


					//copia cada punto descrito por los inices
					for (std::vector<int>::const_iterator pit = it->indices.begin(); pit != it->indices.end(); ++pit)
						cloud_cluster->points.push_back(cloudOut->points[*pit]); 

					cloud_cluster->width = cloud_cluster->points.size();
					cloud_cluster->height = 1;
					cloud_cluster->is_dense = true;

					////////////////////inicia RANSAC

					std::vector<int> inliers;//indice de puntos que mejor dectiben a la figura 
					// se calcula RANSAC para la esfera
					pcl::SampleConsensusModelSphere<PointType>::Ptr model_s(new pcl::SampleConsensusModelSphere<PointType>(cloud_cluster));
					model_s->setRadiusLimits(0.05, 0.7);//5cm-70cm
					pcl::RandomSampleConsensus<PointType> ransacS(model_s);
					ransacS.setDistanceThreshold(.01);//tolerancia de error en la figura
					ransacS.computeModel();
					ransacS.getInliers(inliers);
					probSphere = (double)inliers.size()/ cloud_cluster->width;//se calcula la probablilidad

					// se calcula RANSAC para el plano 
					pcl::SampleConsensusModelPlane<PointType>::Ptr model_p(new pcl::SampleConsensusModelPlane<PointType>(cloud_cluster));
					pcl::RandomSampleConsensus<PointType> ransacP(model_p);
					ransacP.setDistanceThreshold(.01);//tolerancia de error en la figura
					ransacP.computeModel();
					ransacP.getInliers(inliers);
					probPlane = (double)inliers.size()/ cloud_cluster->width;//se calcula la probablilidad

				  
					// se calcula RANSAC para el cilindro 
					pcl::SampleConsensusModelCylinder<PointType, PointNType >::Ptr model_c(new pcl::SampleConsensusModelCylinder<PointType, PointNType >(cloud_cluster));
					//el cilindro ocupa las normales de la figura:
					ne.setSearchMethod(tree);
					ne.setInputCloud(cloud_cluster);
					ne.setKSearch(10);
					ne.compute(*cloud_normals);
					//se agregan los puntos y las normales al modelo
					model_c->setInputNormals(cloud_normals);
					model_c->setInputCloud(cloud_cluster);
					model_c->setRadiusLimits(0.02, 1);//2cm-1m
					pcl::RandomSampleConsensus<PointType> ransacC(model_c);
					ransacC.setDistanceThreshold(.01);//tolerancia de error en la figura
					ransacC.computeModel();
					ransacC.getInliers(inliers);
					probCylinder = (double)inliers.size() / cloud_cluster->width;//se calcula la probablilidad



					//se muestran las probabilidades para cada objeto
					
					std::cout << "sphere prob "<< probSphere << std::endl;
					std::cout << "plane prob " << probPlane << std::endl;
					std::cout << "cylinder prob " << probCylinder << std::endl;
					

					//se elige la mejor probabilidad
					if (probSphere > probCylinder && probSphere > probPlane)
						_sphere = true;
					if (probCylinder > probPlane && probCylinder > probSphere)
						_cylinder = true;
					if (probPlane > probCylinder && probPlane > probSphere)
						_plane = true;



					if (_sphere)
					{
						ransacS.getInliers(inliers);
						//se obtienen los datos de la figura y se convirten en un tipo de dato para graficar
						Eigen::VectorXf coefficients;
						ransacS.getModelCoefficients(coefficients);
						pcl::PointXYZ center;
						center.x = coefficients[0];
						center.y = coefficients[1];
						center.z = coefficients[2];
						//se crea una esfera que correspunde al modelo calculado y se muestra
						viewer2->addSphere(center, coefficients[3], 0.5, 0.0, 0.0, "sphere" + std::to_string(j));

						std::cout << "sphere" + std::to_string(j) << std::endl;
						_sphere = false;

					}
					if (_plane)
					{

						ransacP.getInliers(inliers);//calcula los puntos de la fugura
						//se obtienen los datos de la figura y se convirten en un tipo de dato para graficar
						pcl::ModelCoefficients coefficientsM;
						Eigen::VectorXf coefficients;
						coefficientsM.values.resize(4);
						ransacP.getModelCoefficients(coefficients);
						pcl::PointXYZ center;
						coefficientsM.values[0] = coefficients[0];//centro x
						coefficientsM.values[1] = coefficients[1];//centro y
						coefficientsM.values[2] = coefficients[2];//centro z
						coefficientsM.values[3] = coefficients[3];//radio
						//se crea un plano que correspunde al modelo calculado y se muestra
						viewer2->addPlane(coefficientsM, "plane" + std::to_string(j));

						std::cout << "plane" + std::to_string(j) << std::endl;
						_plane = false;
					}
					if (_cylinder)
					{

						ransacC.getInliers(inliers);
						//se obtienen los datos de la figura y se convirten en un tipo de dato para graficar
						pcl::ModelCoefficients coefficientsM;
						Eigen::VectorXf coefficients;
						coefficientsM.values.resize(7);
						ransacC.getModelCoefficients(coefficients);
						pcl::PointXYZ center;
						coefficientsM.values[0] = coefficients[0];// centro x
						coefficientsM.values[1] = coefficients[1];// centro y
						coefficientsM.values[2] = coefficients[2];// centro z
						coefficientsM.values[3] = coefficients[3];// direccion del eje x
						coefficientsM.values[4] = coefficients[4];// direccion del eje y 
						coefficientsM.values[5] = coefficients[5];// direccion del eje z
						coefficientsM.values[6] = coefficients[6];// radio
						viewer2->addCylinder(coefficientsM, "cylinder" + std::to_string(j));

						std::cout << "cylinder" + std::to_string(j) << std::endl;
						_cylinder = false;
					}

					//se copian solo los puntos de la figura(esfera, plano o cilindro) a otra nube de puntos
					pcl::copyPointCloud<PointType>(*cloud_cluster, inliers, *cloud_cluster2);
					//se agreega la nube resultante a la ventana para la visualizasion
					viewer2->addPointCloud<PointType>(cloud_cluster2, "cloud" + std::to_string(j));
					//se sima uno al contador de cluster
					j++;
					std::cout << "cluster" + std::to_string(j) << std::endl;
				}
			}
			//se envian los datos de las nuves de puntos y mostrarlos en la ventana
			viewer->updatePointCloud(cloudOut, "sample cloud2");
			viewer->updatePointCloud<PointType>(cloudAux, "sample cloud");

			t1 = clock();
			double time = (double(t1 - t0) / CLOCKS_PER_SEC);
			cout << "Sipin time: " << time << endl;
			//si se usa la tecla de ciclo unico
			if(_ciclo2)
			_ciclo = false;
		}
	}

	k2g.shutDown();
	return 0;
}