#include<Game.h>

Game::Game()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;

	int imgFlag = IMG_INIT_PNG;

	if (!(IMG_Init(imgFlag) & imgFlag))
		std::cout << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;

	CreateWindow(TITLE, SCREEN_WIDTH, SCREEN_HEIGHT);

	m_gameRunning = true;
}

Game::~Game()
{
	IMG_Quit();
	SDL_Quit();
}

void Game::CreateWindow(const char* title, int width, int height)
{
	m_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);

	if (m_window == NULL)
		std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;

	m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);

	if (m_renderer == NULL)
		std::cout << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
}

void Game::GameLoop()
{
	while (m_gameRunning)
	{
		m_startTicks = SDL_GetTicks();

		m_newTime = utils::HireTimeInSeconds();
		m_frameTime = m_newTime - m_currentTime;
		m_currentTime = m_newTime;
		m_accumulator += m_frameTime;

		while (m_accumulator >= TIMESTEP)
		{
			while (SDL_PollEvent(&m_event))
			{
				switch (m_event.type)
				{
					case SDL_QUIT:
						m_gameRunning = false;
						break;
					default:
						break;
				}
			}
			m_accumulator -= TIMESTEP;
		}
		m_alpha = m_accumulator / TIMESTEP;

		Update();
		Render();

		m_frameTicks = SDL_GetTicks() - m_startTicks;
		if (m_frameTicks < 1000 / 120)
			SDL_Delay(1000 / 120 - m_frameTicks);
	}
	SDL_DestroyWindow(m_window);
}

void Game::Update()
{
	utils::PrintFps();
}

void Game::Render()
{

}