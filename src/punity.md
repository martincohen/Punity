# DirectSound piece

```
	#define DIRECT_SOUND_CREATE(name) \
	    HRESULT name(LPCGUID pcGuidDevice, LPDIRECTSOUND8 *ppDS, LPUNKNOWN pUnkOuter)
	typedef DIRECT_SOUND_CREATE(DirectSoundCreateF);

	bool
	_win32_sound_init()
	{
	    HMODULE dsound_lib = LoadLibraryA("dsound.dll");
	    if (!dsound_lib)
	    {
	        printf("LoadLibrary(dsound.dll) failed.\n");
	        return 0;
	    }

	    DirectSoundCreateF *DirectSoundCreate =
	        (DirectSoundCreateF *)GetProcAddress(dsound_lib, "DirectSoundCreate8");
	    if (!DirectSoundCreate) {
	        printf("GetProcAddress(DirectSoundCreate) failed.\n");
	        return 0;
	    }

	    LPDIRECTSOUND8 direct_sound;
	    if (FAILED(DirectSoundCreate(0, &direct_sound, 0)))
	    {
	        printf("DirectSoundCreate failed.\n");
	        return 0;
	    }

		
	    if (FAILED(IDirectSound8_SetCooperativeLevel(direct_sound, _win32_window, DSSCL_PRIORITY)))
	    {
	        printf("direct_sound->SetCooperativeLevel failed.\n");
	        return 0;
	    }

	    DSBUFFERDESC primary_buffer_description = {0};
	    primary_buffer_description.dwSize = sizeof(primary_buffer_description);
	    primary_buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER;

	    LPDIRECTSOUNDBUFFER primary_buffer;
	    if(FAILED(IDirectSound8_CreateSoundBuffer(direct_sound, &primary_buffer_description, &primary_buffer, 0)))
	    {
	        printf("direct_sound->CreateSoundBuffer for primary buffer failed.\n");
	        return 0;
	    }

	    WAVEFORMATEX WaveFormat = {0};
	    WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	    WaveFormat.nChannels = 2;
	    WaveFormat.nSamplesPerSec = SOUND_SAMPLE_RATE;
	    WaveFormat.wBitsPerSample = 16;
	    WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
	    WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
	    WaveFormat.cbSize = 0;

	    if (FAILED(IDirectSoundBuffer8_SetFormat(primary_buffer, &WaveFormat)))
	    {
	        printf("primary_buffer->SetFormat failed.");
	        return 0;
	    }

	    DSBUFFERDESC secondary_buffer_description = {0};
	    secondary_buffer_description.dwSize = sizeof(secondary_buffer_description);
	    secondary_buffer_description.dwFlags = 0;
	    secondary_buffer_description.dwBufferBytes = SOUND_SAMPLE_RATE * sizeof(i16) * 2;
	    secondary_buffer_description.lpwfxFormat = &WaveFormat;
	    LPDIRECTSOUNDBUFFER secondary_buffer;
	    if (FAILED(IDirectSound8_CreateSoundBuffer(direct_sound, &secondary_buffer_description, &secondary_buffer, 0)))
	    {
	        printf("direct_sound->CreateSoundBuffer for secondary buffer failed.\n");
	        return 0;
	    }

	    return 1;
	}
```

# WaveOut piece

// https://www.planet-source-code.com/vb/scripts/ShowCode.asp?txtCodeId=4422&lngWId=3
// https://msdn.microsoft.com/en-us/library/windows/desktop/dd797970(v=vs.85).aspx

#define WIN32_WAVE_OUT_PROC(name) \
    void CALLBACK name(HWAVEOUT handle, UINT message, DWORD_PTR instance, \
        DWORD_PTR param1, DWORD_PTR param2);
typedef WIN32_WAVE_OUT_PROC(Win32WaveOutProcF);

static CRITICAL_SECTION win32_wave_out_critical_section;
static i32 win32_wave_out_counter = 0;

void CALLBACK win32_sound_proc(HWAVEOUT handle,
    UINT message, DWORD_PTR instance,
    DWORD_PTR param1, DWORD_PTR param2)
{
    // Ignore calls that occur due to openining and closing the device.
    if (message != WOM_DONE) {
        return;
    }

    EnterCriticalSection(&win32_wave_out_critical_section);
    win32_wave_out_counter++;
    LeaveCriticalSection(&win32_wave_out_critical_section);
}

bool
_win32_sound_init()
{
    WAVEFORMATEX format = {0};
    format.nSamplesPerSec = 48000;
    format.wBitsPerSample = 16;
    format.nChannels = 2;
    format.cbSize = 0;
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nBlockAlign = (format.wBitsPerSample >> 3) * format.nChannels;
    format.nAvgBytesPerSec = format.nBlockAlign * format.nSamplesPerSec;

    HWAVEOUT handle;
    if (waveOutOpen(&handle,
                    WAVE_MAPPER,
                    &format,
                    win32_sound_proc,
                    0L,
                    CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
    {
        printf("waveOutOpen failed.\n");
        return 0;
    }
}