#include <iostream>
#include <fstream>
#include "ReadPPI.h"


ReadPPI::ReadPPI(std::string netname, int num_nets)
{
	m_iNumNets = num_nets;
	igraph_t *m_igraph = new igraph_t[m_iNumNets];
	std::vector<std::string> *m_vecEdges = new std::vector<std::string>[m_iNumNets];
	std::unordered_map<std::string, int> *m_umap_vectex = new std::unordered_map<std::string, int>[m_iNumNets];
	std::unordered_map<int, std::string> *m_umap_pro = new std::unordered_map<int, std::string>[m_iNumNets];
	std::ifstream in(netname);
	if (!in){
		std::cout << "can't open net..." << std::endl;
	}//从文本中读取网络

	std::string temp[3];
	std::vector<std::string> id_nets;
	int i = 0;
	int netId = -1;
	std::string net = "ID";
	std::unordered_map<std::string, int>::iterator ver;
	std::unordered_map<int, std::string>::iterator pro;
	std::cout << "reading network..." << std::endl;
	while (in >> temp[0] && in >> temp[1] && in >> temp[2])
	{
		if (temp[0] != net)
		{
			netId++;
			net = temp[0];
			id_nets.push_back(net);
			i = 0;
		}
		m_vecEdges[netId].push_back(temp[1]);
		m_vecEdges[netId].push_back(temp[2]);
		if (m_umap_vectex[netId].find(temp[1]) == m_umap_vectex[netId].end())
		{
			m_umap_vectex[netId][temp[1]] = i;
			m_umap_pro[netId][i] = temp[1];
			i++;
		}
		if (m_umap_vectex[netId].find(temp[2]) == m_umap_vectex[netId].end())
		{
			m_umap_vectex[netId][temp[2]] = i;
			m_umap_pro[netId][i] = temp[2];
			i++;
		}
	}

	igraph_vector_t *edge_vec = new igraph_vector_t[m_iNumNets];
	for (int p = 0; p < m_iNumNets; p++)
	{
		igraph_vector_init(&edge_vec[p], m_vecEdges[p].size());
		int k = 0;
		for (auto q : m_vecEdges[p])
		{
			VECTOR(edge_vec[p])[k] = m_umap_vectex[p][q];
			k++;
		}
		igraph_create(&m_igraph[p], &edge_vec[p], m_umap_vectex[p].size(), 0);
		igraph_simplify(&m_igraph[p], 1, 1, 0);
		std::cout << "number of vertex in " << id_nets[p] << igraph_vcount(&m_igraph[p]) << std::endl;
		std::cout << "number of edges in " << id_nets[p] << igraph_ecount(&m_igraph[p]) << std::endl;

		const int num_v = igraph_vcount(&m_igraph[p]);
		unsigned short **adj_matrix = new unsigned short*[num_v];
		for (int i = 0; i < num_v; i++)
		{
			adj_matrix[i] = new unsigned short[num_v]();
		}

		unsigned short **adj_matrix_2 = new unsigned short*[num_v];
		for (int i = 0; i < num_v; i++)
		{
			adj_matrix_2[i] = new unsigned short[num_v]();
		}

		double *engin = new double[num_v]();
		igraph_adjlist_t al;
		igraph_adjlist_init(&m_igraph[p], &al, IGRAPH_OUT);

		//初始化邻接矩阵，使用的是数组
		for (int i = 0; i < num_v; i++)
		{
			igraph_vector_int_t *temp;
			temp = igraph_adjlist_get(&al, i);
			int size = igraph_vector_int_size(temp);
			for (int j = 0; j < size; j++)
			{
				adj_matrix[i][VECTOR(*temp)[j]] = 1;
			}
		}

		//计算最大特征值对应的特征向量
		double *te_en = new double[num_v]();
		for (int i = 0; i < num_v; i++)
		{
			for (int j = 0; j < num_v; j++)
			{
				te_en[i] += adj_matrix[j][i];
			}
			//cout << "te_en:" << i << " " << te_en[i] << " " << endl;
		}

		for (int i = 0; i < num_v; i++)
		{
			for (int j = 0; j < num_v; j++)
			{
				if (te_en[j] != 0)
				{
					engin[i] += adj_matrix[i][j] / te_en[j];
				}
			}

		}
		delete[]te_en;
		te_en = NULL;
		/*for (int i = 0; i < num_v; i++)
		{
		mean[0] += engin[i];
		}
		mean[0] = mean[0] / num_v;*/
		//将邻接矩阵自乘，得到两步可达的所有的点

		for (int i = 0; i < num_v; i++)
		{
			for (int j = 0; j < num_v; j++)
			{
				if (adj_matrix[i][j] != 0)
				{
					for (int k = 0; k < num_v; k++)
					{
						if (adj_matrix[j][k] != 0)
						{
							adj_matrix_2[i][k] += adj_matrix[i][j] * adj_matrix[j][k];
						}
					}
				}
			}
		}

		//将每个蛋白质拓扑特征求出来，即一个五维的向量
		for (int i = 0; i < num_v; i++)
		{
			int frist = 0, second = 0;
			std::vector<bool> b_frist(num_v, false);
			double frist_rep = 0, second_rep = 0;
			std::vector<double> temp(5);
			temp[0] = engin[i];

			for (int j = 0; j < num_v; j++)
			{
				if (adj_matrix[i][j] != 0)
				{
					frist++;
					b_frist[j] = true;
					frist_rep += engin[j];
				}
			}
			for (int j = 0; j < num_v; j++)
			{
				if (adj_matrix_2[i][j] != 0)
				{
					if (!b_frist[j] && j != i)
					{
						second++;
						second_rep += engin[j] * adj_matrix_2[i][j] / 2;
					}
				}
			}

			temp[1] = frist;
			temp[2] = frist_rep;
			temp[3] = second;
			temp[4] = second_rep;
			std::string protein = m_umap_pro[p][i];
			top_vec[protein] = temp;
		}

		for (int i = 0; i < num_v; i++)
		{
			delete[]adj_matrix[i];
			adj_matrix[i] = 0;
		}
		delete[]adj_matrix;
		adj_matrix = NULL;

		for (int i = 0; i < num_v; i++)
		{
			delete[]adj_matrix_2[i];
			adj_matrix_2[i] = 0;
		}
		delete[]adj_matrix_2;
		adj_matrix_2 = NULL;

	}
	delete[]m_igraph;
	m_igraph = NULL;
	delete[] m_vecEdges;
	m_vecEdges = NULL;
	delete[] m_umap_pro;
	m_umap_pro = NULL;
	delete[]m_umap_vectex;
	m_umap_vectex = NULL;
}

ReadPPI::~ReadPPI()
{
	

}