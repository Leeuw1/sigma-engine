#pragma once

#include "event/Event.h"

#include <deque>

namespace sge
{
	class Layer
	{
	public:
		virtual void OnAttach() = 0;
		virtual void OnDetach() = 0;
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