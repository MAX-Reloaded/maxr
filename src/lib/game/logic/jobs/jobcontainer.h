/***************************************************************************
 *      Mechanized Assault and Exploration Reloaded Projectfile            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef game_logic_jobs_jobcontainerH
#define game_logic_jobs_jobcontainerH

#include "game/logic/jobs/job.h"
#include "utility/serialization/serialization.h"

#include <memory>

class cJob;
class cUnit;
class cModel;

class cJobContainer
{
public:
	void addJob (cModel&, std::unique_ptr<cJob>);
	void onRemoveUnit (const cUnit&);
	void run (cModel&);
	uint32_t getChecksum (uint32_t crc) const;

	template <typename Archive>
	void serialize (Archive& archive)
	{
		// clang-format off
		// See https://github.com/llvm/llvm-project/issues/44312
		archive & NVP (jobs);
		// clang-format on
	}

	void postLoad (const cModel&);

private:
	std::vector<std::unique_ptr<cJob>>::iterator releaseJob (const cModel&, std::vector<std::unique_ptr<cJob>>::iterator it);

private:
	std::vector<std::unique_ptr<cJob>> jobs;
};

#endif
