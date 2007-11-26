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
#ifndef automjobsH
#define automjobsH

#define WAIT_FRAMES 15

class cEngine;
class cMJobs;
class cVehicle;

class cAutoMJob {


	static cAutoMJob **autoMJobs;		//the active auto move jobs
	static int iCount;					//the number of acive auto move jobs
	static cEngine *engine;				//pointer to the engine
	
	cMJobs *lastMoveJob;	//pointer to the last mjob, generated by the AI.
							//needed to check if the move job was changed from outside the AI (i. e. by the player)
	cVehicle *vehicle;		//the vehicle the auto move job belongs to
	int iNumber;			//index of the AutoMJob in autoMJobs[]
	int n;					//debug
	
	void DoAutoMove();
	float CalcFactor(int x, int y); 
	

public:
	static void init(cEngine* engine);
	static void handleAutoMoveJobs();
	cAutoMJob(cVehicle *vehicle);
	~cAutoMJob(void);

};

#endif  //automjobsH
