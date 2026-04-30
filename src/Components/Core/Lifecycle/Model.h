#ifndef _INERTIA_COMPONENTS_LIFECYCLE_MODEL_h
#define _INERTIA_COMPONENTS_LIFECYCLE_MODEL_h

#include "../../../Framework/Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace Lifecycle
		{
			/// <summary>
			/// Defines an interface for managing the lifecycle of an object with start and stop operations.
			/// </summary>
			struct ILifecycleDriver
			{
				~ILifecycleDriver() = default;

				/// <summary>
				/// Starts the object. Pure virtual function that must be implemented by derived classes.
				/// </summary>
				/// <returns>true if the start succeeded; otherwise false.</returns>
				virtual bool Start() = 0;

				/// <summary>
				/// Stop the object, regardless of current state. Pure virtual function that must be implemented by derived classes.
				/// </summary>
				virtual void Stop() = 0;
			};

			/// <summary>
			/// Generic driver interface with basic lifecycle and step method for periodic update.
			/// </summary>
			struct IPeriodicDriver : public ILifecycleDriver
			{
				~IPeriodicDriver() = default;

				/// <summary>
				/// Generic call for periodic step/update. Pure virtual function that must be implemented by derived classes.
				/// </summary>
				virtual void Step() = 0;
			};
		}
	}
}

#endif
