#include "Layer.h"

namespace sge
{
	LayerStack::~LayerStack()
	{
		for (auto layer : m_Layers)
			delete layer;
	}

	void LayerStack::OnUpdate()
	{
		for (auto layer : m_Layers)
			layer->OnUpdate();
	}

	void LayerStack::OnEvent(Event& event)
	{
		for (auto it = m_Layers.end(); it != m_Layers.begin();)
		{
			it--;
			// Stop if event was handled
			if (it.operator*()->OnEvent(event))
				return;
		}
	}

	void LayerStack::PushFront(Layer* layer)
	{
		m_Layers.push_front(layer);
	}

	void LayerStack::PushBack(Layer* layer)
	{
		m_Layers.push_back(layer);
	}
}