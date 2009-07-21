/**
 * KX_SoundActuator.cpp
 *
 * $Id$
 *
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "KX_SoundActuator.h"
#include "SND_SoundObject.h"
#include "KX_GameObject.h"
#include "SND_SoundObject.h"
#include "SND_Scene.h" // needed for replication
#include "KX_PyMath.h" // needed for PyObjectFrom()
#include <iostream>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* ------------------------------------------------------------------------- */
/* Native functions                                                          */
/* ------------------------------------------------------------------------- */
KX_SoundActuator::KX_SoundActuator(SCA_IObject* gameobj,
								   AUD_Sound* sound,
// AUD_XXX								   SND_SoundObject* sndobj,
// AUD_XXX								   SND_Scene*	sndscene,
								   KX_SOUNDACT_TYPE type,
								   short start,
								   short end)
								   : SCA_IActuator(gameobj)
{
// AUD_XXX	m_soundObject = sndobj;
// AUD_XXX	m_soundScene = sndscene;
	m_sound = sound;
	m_handle = NULL;
	m_type = type;
	m_lastEvent = true;
	m_isplaying = false;
	m_startFrame = start;
	m_endFrame = end;
	m_pino = false;


}



KX_SoundActuator::~KX_SoundActuator()
{
// AUD_XXX
	if(m_handle)
		AUD_stop(m_handle);
	if(m_sound)
		AUD_unload(m_sound);
#if 0
	if (m_soundObject)
	{
		m_soundScene->RemoveActiveObject(m_soundObject);
		m_soundScene->DeleteObject(m_soundObject);
	}
#endif
}



CValue* KX_SoundActuator::GetReplica()
{
	KX_SoundActuator* replica = new KX_SoundActuator(*this);
	replica->ProcessReplica();
	return replica;
};

void KX_SoundActuator::ProcessReplica()
{
	SCA_IActuator::ProcessReplica();
// AUD_XXX
#if 0
	if (m_soundObject)
	{
		SND_SoundObject* soundobj = new SND_SoundObject(*m_soundObject);
		setSoundObject(soundobj);
		m_soundScene->AddObject(soundobj);
	}
#endif
}

bool KX_SoundActuator::Update(double curtime, bool frame)
{
	if (!frame)
		return true;
	bool result = false;

	// do nothing on negative events, otherwise sounds are played twice!
	bool bNegativeEvent = IsNegativeEvent();

	RemoveAllEvents();

// AUD_XXX	if (!m_soundObject)
	if(!m_sound)
		return false;

	// actual audio device playing state
	bool isplaying = /*AUD_XXX(m_soundObject->GetPlaystate() != SND_STOPPED && m_soundObject->GetPlaystate() != SND_INITIAL) ? true : false*/ AUD_getStatus(m_handle) == AUD_STATUS_PLAYING;

	if (m_pino)
	{
		bNegativeEvent = true;
		m_pino = false;
	}

	if (bNegativeEvent)
	{
		// here must be a check if it is still playing
		if (m_isplaying && isplaying)
		{
			switch (m_type)
			{
			case KX_SOUNDACT_PLAYSTOP:
			case KX_SOUNDACT_LOOPSTOP:
			case KX_SOUNDACT_LOOPBIDIRECTIONAL_STOP:
				{
// AUD_XXX					m_soundScene->RemoveActiveObject(m_soundObject);
					AUD_stop(m_handle);
					break;
				}
			case KX_SOUNDACT_PLAYEND:
				{
// AUD_XXX					m_soundObject->SetPlaystate(SND_MUST_STOP_WHEN_FINISHED);
					break;
				}
			case KX_SOUNDACT_LOOPEND:
			case KX_SOUNDACT_LOOPBIDIRECTIONAL:
				{
// AUD_XXX					m_soundObject->SetLoopMode(SND_LOOP_OFF);
// AUD_XXX					m_soundObject->SetPlaystate(SND_MUST_STOP_WHEN_FINISHED);
// AUD_XXX					AUD_setEndBehaviour(m_handle, AUD_BEHAVIOUR_STOP);
					break;
				}
			default:
				// implement me !!
				break;
			}
		}
		// remember that we tried to stop the actuator
		m_isplaying = false;
	}
	else
	{
		if (!m_isplaying)
		{
			switch (m_type)
			{
// AUD_XXX
#if 0
			case KX_SOUNDACT_LOOPBIDIRECTIONAL:
			case KX_SOUNDACT_LOOPBIDIRECTIONAL_STOP:
				{
					m_soundObject->SetLoopMode(SND_LOOP_BIDIRECTIONAL);
					m_soundScene->AddActiveObject(m_soundObject, curtime);
					m_isplaying = true;
					result = true;
					break;
				}
			case KX_SOUNDACT_LOOPEND:
			case KX_SOUNDACT_LOOPSTOP:
				{
					m_soundObject->SetLoopMode(SND_LOOP_NORMAL);
					m_soundScene->AddActiveObject(m_soundObject, curtime);
					m_isplaying = true;
					result = true;
					break;
				}
			case KX_SOUNDACT_PLAYSTOP:
			case KX_SOUNDACT_PLAYEND:
				{
					m_soundObject->SetLoopMode(SND_LOOP_OFF);
					m_soundScene->AddActiveObject(m_soundObject, curtime);
					m_isplaying = true;
					result = true;
					break;
				}
#else
			case KX_SOUNDACT_LOOPBIDIRECTIONAL:
			case KX_SOUNDACT_LOOPBIDIRECTIONAL_STOP:
			case KX_SOUNDACT_LOOPEND:
			case KX_SOUNDACT_LOOPSTOP:
				{
					if(m_handle)
						AUD_stop(m_handle);
					m_handle = AUD_play(m_sound, 0);
					m_isplaying = true;
					result = true;
					break;
				}
			case KX_SOUNDACT_PLAYSTOP:
			case KX_SOUNDACT_PLAYEND:
				{
					if(m_handle)
						AUD_stop(m_handle);
					m_handle = AUD_play(m_sound, 0);
					m_isplaying = true;
					result = true;
					break;
				}
#endif
			default:
				// implement me !!
				break;
			}
		}
	}
	// verify that the sound is still playing
	isplaying = /*AUD_XXX(m_soundObject->GetPlaystate() != SND_STOPPED && m_soundObject->GetPlaystate() != SND_INITIAL) ? true :*/ AUD_getStatus(m_handle) == AUD_STATUS_PLAYING ? true : false;

	if (isplaying)
	{
// AUD_XXX
#if 0
		m_soundObject->SetPosition(((KX_GameObject*)this->GetParent())->NodeGetWorldPosition());
		m_soundObject->SetVelocity(((KX_GameObject*)this->GetParent())->GetLinearVelocity());
		m_soundObject->SetOrientation(((KX_GameObject*)this->GetParent())->NodeGetWorldOrientation());
#endif
		result = true;
	}
	else
	{
		m_isplaying = false;
		result = false;
	}
	/*
	if (result && (m_soundObject->IsLifeSpanOver(curtime)) && ((m_type == KX_SOUNDACT_PLAYEND) || (m_type == KX_SOUNDACT_PLAYSTOP)))
	{
		m_pino = true;
	}
	*/
	return result;
}


// AUD_XXX
#if 0
void KX_SoundActuator::setSoundObject(class SND_SoundObject* soundobject)
{
	m_soundObject = soundobject;
}
#endif



/* ------------------------------------------------------------------------- */
/* Python functions                                                          */
/* ------------------------------------------------------------------------- */



/* Integration hooks ------------------------------------------------------- */
PyTypeObject KX_SoundActuator::Type = {
#if (PY_VERSION_HEX >= 0x02060000)
	PyVarObject_HEAD_INIT(NULL, 0)
#else
	/* python 2.5 and below */
	PyObject_HEAD_INIT( NULL )  /* required py macro */
	0,                          /* ob_size */
#endif
		"KX_SoundActuator",
		sizeof(PyObjectPlus_Proxy),
		0,
		py_base_dealloc,
		0,
		0,
		0,
		0,
		py_base_repr,
		0,0,0,0,0,0,0,0,0,
		Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
		0,0,0,0,0,0,0,
		Methods,
		0,
		0,
		&SCA_IActuator::Type,
		0,0,0,0,0,0,
		py_base_new
};

PyMethodDef KX_SoundActuator::Methods[] = {
	// Deprecated ----->
	{"setFilename", (PyCFunction) KX_SoundActuator::sPySetFilename, METH_VARARGS,NULL},
	{"getFilename", (PyCFunction) KX_SoundActuator::sPyGetFilename, METH_NOARGS,NULL},
	{"setGain",(PyCFunction) KX_SoundActuator::sPySetGain,METH_VARARGS,NULL},
	{"getGain",(PyCFunction) KX_SoundActuator::sPyGetGain,METH_NOARGS,NULL},
	{"setPitch",(PyCFunction) KX_SoundActuator::sPySetPitch,METH_VARARGS,NULL},
	{"getPitch",(PyCFunction) KX_SoundActuator::sPyGetPitch,METH_NOARGS,NULL},
	{"setRollOffFactor",(PyCFunction) KX_SoundActuator::sPySetRollOffFactor,METH_VARARGS,NULL},
	{"getRollOffFactor",(PyCFunction) KX_SoundActuator::sPyGetRollOffFactor,METH_NOARGS,NULL},
	{"setLooping",(PyCFunction) KX_SoundActuator::sPySetLooping,METH_VARARGS,NULL},
	{"getLooping",(PyCFunction) KX_SoundActuator::sPyGetLooping,METH_NOARGS,NULL},
	{"setPosition",(PyCFunction) KX_SoundActuator::sPySetPosition,METH_VARARGS,NULL},
	{"setVelocity",(PyCFunction) KX_SoundActuator::sPySetVelocity,METH_VARARGS,NULL},
	{"setOrientation",(PyCFunction) KX_SoundActuator::sPySetOrientation,METH_VARARGS,NULL},
	{"setType",(PyCFunction) KX_SoundActuator::sPySetType,METH_VARARGS,NULL},
	{"getType",(PyCFunction) KX_SoundActuator::sPyGetType,METH_NOARGS,NULL},
	// <-----

	KX_PYMETHODTABLE_NOARGS(KX_SoundActuator, startSound),
	KX_PYMETHODTABLE_NOARGS(KX_SoundActuator, pauseSound),
	KX_PYMETHODTABLE_NOARGS(KX_SoundActuator, stopSound),
	{NULL,NULL,NULL,NULL} //Sentinel
};

PyAttributeDef KX_SoundActuator::Attributes[] = {
	KX_PYATTRIBUTE_RW_FUNCTION("fileName", KX_SoundActuator, pyattr_get_filename, pyattr_set_filename),
	KX_PYATTRIBUTE_RW_FUNCTION("volume", KX_SoundActuator, pyattr_get_gain, pyattr_set_gain),
	KX_PYATTRIBUTE_RW_FUNCTION("pitch", KX_SoundActuator, pyattr_get_pitch, pyattr_set_pitch),
	KX_PYATTRIBUTE_RW_FUNCTION("rollOffFactor", KX_SoundActuator, pyattr_get_rollOffFactor, pyattr_set_rollOffFactor),
	KX_PYATTRIBUTE_RW_FUNCTION("looping", KX_SoundActuator, pyattr_get_looping, pyattr_set_looping),
	KX_PYATTRIBUTE_RW_FUNCTION("position", KX_SoundActuator, pyattr_get_position, pyattr_set_position),
	KX_PYATTRIBUTE_RW_FUNCTION("velocity", KX_SoundActuator, pyattr_get_velocity, pyattr_set_velocity),
	KX_PYATTRIBUTE_RW_FUNCTION("orientation", KX_SoundActuator, pyattr_get_orientation, pyattr_set_orientation),
	KX_PYATTRIBUTE_ENUM_RW("mode",KX_SoundActuator::KX_SOUNDACT_NODEF+1,KX_SoundActuator::KX_SOUNDACT_MAX-1,false,KX_SoundActuator,m_type),
	{ NULL }	//Sentinel
};

/* Methods ----------------------------------------------------------------- */
KX_PYMETHODDEF_DOC_NOARGS(KX_SoundActuator, startSound,
"startSound()\n"
"\tStarts the sound.\n")
{
// AUD_XXX	if (m_soundObject)
		// This has no effect if the actuator is not active.
		// To start the sound you must activate the actuator.
		// This function is to restart the sound.
// AUD_XXX		m_soundObject->StartSound();
	switch(AUD_getStatus(m_handle))
	{
	case AUD_STATUS_PLAYING:
		break;
	case AUD_STATUS_PAUSED:
		AUD_resume(m_handle);
		break;
	default:
		if(m_handle)
			AUD_stop(m_handle);
		m_handle = AUD_play(m_sound, 0);
	}
	Py_RETURN_NONE;
}

KX_PYMETHODDEF_DOC_NOARGS(KX_SoundActuator, pauseSound,
"pauseSound()\n"
"\tPauses the sound.\n")
{
// AUD_XXX	if (m_soundObject)
		// unfortunately, openal does not implement pause correctly, it is equivalent to a stop
// AUD_XXX		m_soundObject->PauseSound();
	AUD_pause(m_handle);
	Py_RETURN_NONE;
}

KX_PYMETHODDEF_DOC_NOARGS(KX_SoundActuator, stopSound,
"stopSound()\n"
"\tStops the sound.\n")
{
// AUD_XXX	if (m_soundObject)
// AUD_XXX		m_soundObject->StopSound();
	AUD_stop(m_handle);
	Py_RETURN_NONE;
}

/* Atribute setting and getting -------------------------------------------- */

PyObject* KX_SoundActuator::pyattr_get_filename(void *self, const struct KX_PYATTRIBUTE_DEF *attrdef)
{
	KX_SoundActuator * actuator = static_cast<KX_SoundActuator *> (self);
// AUD_XXX	if (!actuator->m_soundObject)
// AUD_XXX	{
		return PyUnicode_FromString("");
// AUD_XXX
#if 0
	}
	STR_String objectname = actuator->m_soundObject->GetObjectName();
	char* name = objectname.Ptr();

	if (!name) {
		PyErr_SetString(PyExc_RuntimeError, "value = actuator.fileName: KX_SoundActuator, unable to get sound fileName");
		return NULL;
	} else
		return PyUnicode_FromString(name);
#endif
}

PyObject* KX_SoundActuator::pyattr_get_gain(void *self, const struct KX_PYATTRIBUTE_DEF *attrdef)
{
	KX_SoundActuator * actuator = static_cast<KX_SoundActuator *> (self);
	float gain = /*AUD_XXX(actuator->m_soundObject) ? actuator->m_soundObject->GetGain() :*/ 1.0f;

	PyObject* result = PyFloat_FromDouble(gain);

	return result;
}

PyObject* KX_SoundActuator::pyattr_get_pitch(void *self, const struct KX_PYATTRIBUTE_DEF *attrdef)
{
	KX_SoundActuator * actuator = static_cast<KX_SoundActuator *> (self);
	float pitch = /*AUD_XXX(actuator->m_soundObject) ? actuator->m_soundObject->GetPitch() :*/ 1.0;
	PyObject* result = PyFloat_FromDouble(pitch);

	return result;
}

PyObject* KX_SoundActuator::pyattr_get_rollOffFactor(void *self, const struct KX_PYATTRIBUTE_DEF *attrdef)
{
	KX_SoundActuator * actuator = static_cast<KX_SoundActuator *> (self);
	float rollofffactor = /*AUD_XXX(actuator->m_soundObject) ? actuator->m_soundObject->GetRollOffFactor() :*/ 1.0;
	PyObject* result = PyFloat_FromDouble(rollofffactor);

	return result;
}

PyObject* KX_SoundActuator::pyattr_get_looping(void *self, const struct KX_PYATTRIBUTE_DEF *attrdef)
{
	KX_SoundActuator * actuator = static_cast<KX_SoundActuator *> (self);
	int looping = /*AUD_XXX(actuator->m_soundObject) ? actuator->m_soundObject->GetLoopMode() :*/ (int)SND_LOOP_OFF;
	PyObject* result = PyLong_FromSsize_t(looping);

	return result;
}

PyObject* KX_SoundActuator::pyattr_get_position(void * self, const struct KX_PYATTRIBUTE_DEF *attrdef)
{
	KX_SoundActuator * actuator = static_cast<KX_SoundActuator *> (self);
	MT_Vector3 pos(0.0, 0.0, 0.0);

// AUD_XXX	if (actuator->m_soundObject)
// AUD_XXX		pos = actuator->m_soundObject->GetPosition();

	PyObject * result = PyObjectFrom(pos);
	return result;
}

PyObject* KX_SoundActuator::pyattr_get_velocity(void *self, const struct KX_PYATTRIBUTE_DEF *attrdef)
{
	KX_SoundActuator * actuator = static_cast<KX_SoundActuator *> (self);
	MT_Vector3 vel;

// AUD_XXX	if (actuator->m_soundObject)
// AUD_XXX		vel = actuator->m_soundObject->GetVelocity();

	PyObject * result = PyObjectFrom(vel);
	return result;
}

PyObject* KX_SoundActuator::pyattr_get_orientation(void *self, const struct KX_PYATTRIBUTE_DEF *attrdef)
{
	KX_SoundActuator * actuator = static_cast<KX_SoundActuator *> (self);
	MT_Matrix3x3 ori;

// AUD_XXX	if (actuator->m_soundObject)
// AUD_XXX		ori = actuator->m_soundObject->GetOrientation();

	PyObject * result = PyObjectFrom(ori);
	return result;
}

int KX_SoundActuator::pyattr_set_filename(void *self, const struct KX_PYATTRIBUTE_DEF *attrdef, PyObject *value)
{
	char *soundName = NULL;
	KX_SoundActuator * actuator = static_cast<KX_SoundActuator*> (self);
	// void *soundPointer = NULL; /*unused*/

	if (!PyArg_Parse(value, "s", &soundName))
		return PY_SET_ATTR_FAIL;

// AUD_XXX	if (actuator->m_soundObject) {
// AUD_XXX		actuator->m_soundObject->SetObjectName(soundName);
// AUD_XXX	}

	return PY_SET_ATTR_SUCCESS;
}


int KX_SoundActuator::pyattr_set_gain(void *self, const struct KX_PYATTRIBUTE_DEF *attrdef, PyObject *value)
{
	float gain = 1.0;
	KX_SoundActuator * actuator = static_cast<KX_SoundActuator *> (self);
	if (!PyArg_Parse(value, "f", &gain))
		return PY_SET_ATTR_FAIL;

// AUD_XXX	if (actuator->m_soundObject)
// AUD_XXX		actuator->m_soundObject->SetGain(gain);

	return PY_SET_ATTR_SUCCESS;
}

int KX_SoundActuator::pyattr_set_pitch(void *self, const struct KX_PYATTRIBUTE_DEF *attrdef, PyObject *value)
{
	float pitch = 1.0;
	KX_SoundActuator * actuator = static_cast<KX_SoundActuator *> (self);
	if (!PyArg_Parse(value, "f", &pitch))
		return PY_SET_ATTR_FAIL;

// AUD_XXX	if (actuator->m_soundObject)
// AUD_XXX		actuator->m_soundObject->SetPitch(pitch);

	return PY_SET_ATTR_SUCCESS;
}

int KX_SoundActuator::pyattr_set_rollOffFactor(void *self, const struct KX_PYATTRIBUTE_DEF *attrdef, PyObject *value)
{
	KX_SoundActuator * actuator = static_cast<KX_SoundActuator *> (self);
	float rollofffactor = 1.0;
	if (!PyArg_Parse(value, "f", &rollofffactor))
		return PY_SET_ATTR_FAIL;

// AUD_XXX	if (actuator->m_soundObject)
// AUD_XXX		actuator->m_soundObject->SetRollOffFactor(rollofffactor);

	return PY_SET_ATTR_SUCCESS;
}

int KX_SoundActuator::pyattr_set_looping(void *self, const struct KX_PYATTRIBUTE_DEF *attrdef, PyObject *value)
{
	KX_SoundActuator * actuator = static_cast<KX_SoundActuator *> (self);
	int looping = 1;
	if (!PyArg_Parse(value, "i", &looping))
		return PY_SET_ATTR_FAIL;

// AUD_XXX	if (actuator->m_soundObject)
// AUD_XXX		actuator->m_soundObject->SetLoopMode(looping);

	return PY_SET_ATTR_SUCCESS;
}

int KX_SoundActuator::pyattr_set_position(void *self, const struct KX_PYATTRIBUTE_DEF *attrdef, PyObject *value)
{
	float pos[3];

	KX_SoundActuator * actuator = static_cast<KX_SoundActuator *> (self);

	if (!PyArg_ParseTuple(value, "fff", &pos[0], &pos[1], &pos[2]))
		return PY_SET_ATTR_FAIL;

// AUD_XXX	if (actuator->m_soundObject)
// AUD_XXX		actuator->m_soundObject->SetPosition(MT_Vector3(pos));

	return PY_SET_ATTR_SUCCESS;
}

int KX_SoundActuator::pyattr_set_velocity(void *self, const struct KX_PYATTRIBUTE_DEF *attrdef, PyObject *value)
{
	float vel[3];
	KX_SoundActuator * actuator = static_cast<KX_SoundActuator *> (self);


	if (!PyArg_ParseTuple(value, "fff", &vel[0], &vel[1], &vel[2]))
		return PY_SET_ATTR_FAIL;

// AUD_XXX	if (actuator->m_soundObject)
// AUD_XXX		actuator->m_soundObject->SetVelocity(MT_Vector3(vel));

	return PY_SET_ATTR_SUCCESS;

}

int KX_SoundActuator::pyattr_set_orientation(void *self, const struct KX_PYATTRIBUTE_DEF *attrdef, PyObject *value)
{

	MT_Matrix3x3 rot;
	KX_SoundActuator * actuator = static_cast<KX_SoundActuator *> (self);

	/* if value is not a sequence PyOrientationTo makes an error */
	if (!PyOrientationTo(value, rot, "actuator.orientation = value: KX_SoundActuator"))
		return PY_SET_ATTR_FAIL;

	/* Since not having m_soundObject didn't do anything in the old version,
	 * it probably should be kept that way  */
// AUD_XXX	if (!actuator->m_soundObject)
		return PY_SET_ATTR_SUCCESS;

// AUD_XXX	actuator->m_soundObject->SetOrientation(rot);
// AUD_XXX	return PY_SET_ATTR_SUCCESS;
}

// Deprecated ----->
PyObject* KX_SoundActuator::PySetFilename(PyObject* args)
{
	char *soundName = NULL;
	ShowDeprecationWarning("setFilename()", "the fileName property");
	// void *soundPointer = NULL; /*unused*/

	if (!PyArg_ParseTuple(args, "s", &soundName))
		return NULL;

	Py_RETURN_NONE;
}

PyObject* KX_SoundActuator::PyGetFilename()
{
	ShowDeprecationWarning("getFilename()", "the fileName property");
// AUD_XXX	if (!m_soundObject)
// AUD_XXX	{
		return PyUnicode_FromString("");
// AUD_XXX
#if 0
	}
	STR_String objectname = m_soundObject->GetObjectName();
	char* name = objectname.Ptr();

	if (!name) {
		PyErr_SetString(PyExc_RuntimeError, "Unable to get sound fileName");
		return NULL;
	} else
		return PyUnicode_FromString(name);
#endif
}

PyObject* KX_SoundActuator::PySetGain(PyObject* args)
{
	ShowDeprecationWarning("setGain()", "the volume property");
	float gain = 1.0;
	if (!PyArg_ParseTuple(args, "f:setGain", &gain))
		return NULL;

// AUD_XXX	if (m_soundObject)
// AUD_XXX		m_soundObject->SetGain(gain);

	Py_RETURN_NONE;
}



PyObject* KX_SoundActuator::PyGetGain()
{
	ShowDeprecationWarning("getGain()", "the volume property");
	float gain = /*AUD_XXX(m_soundObject) ? m_soundObject->GetGain() :*/ 1.0f;
	PyObject* result = PyFloat_FromDouble(gain);

	return result;
}



PyObject* KX_SoundActuator::PySetPitch(PyObject* args)
{
	ShowDeprecationWarning("setPitch()", "the pitch property");
	float pitch = 1.0;
	if (!PyArg_ParseTuple(args, "f:setPitch", &pitch))
		return NULL;

// AUD_XXX	if (m_soundObject)
// AUD_XXX		m_soundObject->SetPitch(pitch);

	Py_RETURN_NONE;
}



PyObject* KX_SoundActuator::PyGetPitch()
{
	ShowDeprecationWarning("getPitch()", "the pitch property");
	float pitch = /*AUD_XXX(m_soundObject) ? m_soundObject->GetPitch() :*/ 1.0;
	PyObject* result = PyFloat_FromDouble(pitch);

	return result;
}



PyObject* KX_SoundActuator::PySetRollOffFactor(PyObject* args)
{
	ShowDeprecationWarning("setRollOffFactor()", "the rollOffFactor property");
	float rollofffactor = 1.0;
	if (!PyArg_ParseTuple(args, "f:setRollOffFactor", &rollofffactor))
		return NULL;

// AUD_XXX	if (m_soundObject)
// AUD_XXX		m_soundObject->SetRollOffFactor(rollofffactor);

	Py_RETURN_NONE;
}



PyObject* KX_SoundActuator::PyGetRollOffFactor()
{
	ShowDeprecationWarning("getRollOffFactor()", "the rollOffFactor property");
	float rollofffactor = /*AUD_XXX(m_soundObject) ? m_soundObject->GetRollOffFactor() :*/ 1.0;
	PyObject* result = PyFloat_FromDouble(rollofffactor);

	return result;
}



PyObject* KX_SoundActuator::PySetLooping(PyObject* args)
{
	ShowDeprecationWarning("setLooping()", "the looping property");
	bool looping = 1;
	if (!PyArg_ParseTuple(args, "i:setLooping", &looping))
		return NULL;

// AUD_XXX	if (m_soundObject)
// AUD_XXX		m_soundObject->SetLoopMode(looping);

	Py_RETURN_NONE;
}



PyObject* KX_SoundActuator::PyGetLooping()
{
	ShowDeprecationWarning("getLooping()", "the looping property");
	int looping = /*AUD_XXX(m_soundObject) ? m_soundObject->GetLoopMode() :*/ (int)SND_LOOP_OFF;
	PyObject* result = PyLong_FromSsize_t(looping);

	return result;
}



PyObject* KX_SoundActuator::PySetPosition(PyObject* args)
{
	MT_Point3 pos;
	ShowDeprecationWarning("setPosition()", "the position property");
	pos[0] = 0.0;
	pos[1] = 0.0;
	pos[2] = 0.0;

	if (!PyArg_ParseTuple(args, "fff:setPosition", &pos[0], &pos[1], &pos[2]))
		return NULL;

// AUD_XXX	if (m_soundObject)
// AUD_XXX		m_soundObject->SetPosition(pos);

	Py_RETURN_NONE;
}



PyObject* KX_SoundActuator::PySetVelocity(PyObject* args)
{
	MT_Vector3 vel;
	ShowDeprecationWarning("setVelocity()", "the velocity property");
	vel[0] = 0.0;
	vel[1] = 0.0;
	vel[2] = 0.0;

	if (!PyArg_ParseTuple(args, "fff:setVelocity", &vel[0], &vel[1], &vel[2]))
		return NULL;

// AUD_XXX	if (m_soundObject)
// AUD_XXX		m_soundObject->SetVelocity(vel);

	Py_RETURN_NONE;
}



PyObject* KX_SoundActuator::PySetOrientation(PyObject* args)
{
	MT_Matrix3x3 ori;
	ShowDeprecationWarning("setOrientation()", "the orientation property");
	ori[0][0] = 1.0;
	ori[0][1] = 0.0;
	ori[0][2] = 0.0;
	ori[1][0] = 0.0;
	ori[1][1] = 1.0;
	ori[1][2] = 0.0;
	ori[2][0] = 0.0;
	ori[2][1] = 0.0;
	ori[2][2] = 1.0;

	if (!PyArg_ParseTuple(args, "fffffffff:setOrientation", &ori[0][0], &ori[0][1], &ori[0][2], &ori[1][0], &ori[1][1], &ori[1][2], &ori[2][0], &ori[2][1], &ori[2][2]))
		return NULL;

// AUD_XXX	if (m_soundObject)
// AUD_XXX		m_soundObject->SetOrientation(ori);

	Py_RETURN_NONE;
}

PyObject* KX_SoundActuator::PySetType(PyObject* args)
{
	int typeArg;
	ShowDeprecationWarning("setType()", "the mode property");

	if (!PyArg_ParseTuple(args, "i:setType", &typeArg)) {
		return NULL;
	}

	if ( (typeArg > KX_SOUNDACT_NODEF)
	  && (typeArg < KX_SOUNDACT_MAX) ) {
		m_type = (KX_SOUNDACT_TYPE) typeArg;
	}

	Py_RETURN_NONE;
}

PyObject* KX_SoundActuator::PyGetType()
{
	ShowDeprecationWarning("getType()", "the mode property");
	return PyLong_FromSsize_t(m_type);
}
// <-----

