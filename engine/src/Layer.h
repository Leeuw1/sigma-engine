#pragma once

#include "event/Event.h"

#include <deque>

// This style of Layers and Layer stack was inspired by Hazel

namespace sge
{
	class Layer
	{
	public:
		virtual ~Layer() = default;
		virtual void OnUpdate() = 0;
		virtual bool OnEvent(Event& event) = 0;
	};

	class LayerStack
	{
	private:
		// Layers should be allocated with operator new
		std::deque<Layer*> m_Layers;
	public:
		~LayerStack();
		void PushFront(Layer* layer);
		void PushBack(Layer* layer);
		void OnUpdate();
		// Return true if handled
		void OnEvent(Event& event);
	};
}