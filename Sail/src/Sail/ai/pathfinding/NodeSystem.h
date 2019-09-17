#pragma once

#include <map>
#include <list>

#define _DEBUG_NODESYSTEM

#ifdef _DEBUG_NODESYSTEM
#include "Sail/entities/Entity.h"
#endif

class Model;
class Scene;

class NodeSystem {
public:
	struct Node {
		glm::vec3 position;
		unsigned int index;

		Node(glm::vec3 pos, unsigned int i)
			: position(pos)
			, index(i)
		{}
	};

private:
	class Graph {
	public:
		explicit Graph(const unsigned int numNodes);
		~Graph();

		void addEdge(const unsigned int src, const unsigned int dest);
		std::vector<unsigned int> BFS(const unsigned int from, const unsigned int to);

	private:
		std::vector<std::list<unsigned int>> m_connections;
	};

public:
	NodeSystem();
	~NodeSystem();

	void setNodes(const std::vector<glm::vec3>& nodes, const std::vector<std::vector<unsigned int>>& connections);

	std::vector<NodeSystem::Node> getPath(const NodeSystem::Node& from, const NodeSystem::Node& to);
	std::vector<NodeSystem::Node> getPath(const glm::vec3& from, const glm::vec3& to);

	const NodeSystem::Node& getNearestNode(const glm::vec3& position) const;

#ifdef _DEBUG_NODESYSTEM
	void setDebugModelAndScene(Model* model, Scene* scene);
#endif

private:
	std::vector<NodeSystem::Node> m_nodes;

	NodeSystem::Graph* m_graph;

#ifdef _DEBUG_NODESYSTEM
	Model* m_nodeModel;
	Scene* m_scene;
	std::vector<Entity::SPtr> m_nodeEntities;
#endif
};