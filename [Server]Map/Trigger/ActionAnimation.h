#pragma once
#include "Action.h"

namespace Trigger
{
	class CActionAnimation :
		public CAction
	{
	public:
		virtual void DoAction();
		static CAction* Clone()
		{
			return new CActionAnimation;
		}
	};
}