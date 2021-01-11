/*
 * Semaphore.cpp
 *
 * Copyright (C) 2011 IBR, TU Braunschweig
 *
 * Written-by: Johannes Morgenroth <morgenroth@ibr.cs.tu-bs.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "ibrcommon/config.h"
#include "ibrcommon/thread/Semaphore.h"
#include <iostream>

using namespace std;

namespace ibrcommon
{
	Semaphore::Semaphore(std::string name, unsigned int value) : name(name.c_str())
	{
		errno = 0;
		count_sem = sem_open(name.c_str(), O_CREAT | O_EXCL, 0644, value);
		if(SEM_FAILED == count_sem)
		{
			throw MutexException("error whilst creating semaphore");
		}
	}

	Semaphore::~Semaphore()
	{
		sem_close(count_sem);
		sem_unlink(name);
	}

	void Semaphore::wait()
	{
		sem_wait(count_sem);
	}

	void Semaphore::post()
	{
		sem_post(count_sem);
	}

	void Semaphore::trylock() throw (MutexException)
	{
		throw MutexException("trylock is not available for semaphores");
	}

	void Semaphore::enter() throw (MutexException)
	{
		wait();
	}

	void Semaphore::leave() throw (MutexException)
	{
		post();
	}
}
