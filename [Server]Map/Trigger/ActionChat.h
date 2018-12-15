#pragma once
#include "Action.h"

namespace Trigger
{
	class CActionChat :
		public CAction
	{
	public:
		virtual void DoAction();
		static CAction* Clone()
		{
			return new CActionChat;
		}
	};
}