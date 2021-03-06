#include<Game.h>

Game::Game()
	:m_assetManager(std::make_unique<AssetManager>())
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;

	int imgFlag = IMG_INIT_PNG;

	if (!(IMG_Init(imgFlag) & imgFlag))
		std::cout << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;

	CreateWindow(TITLE, SCREEN_WIDTH, SCREEN_HEIGHT);

	//setup ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer);
	ImGui_ImplSDLRenderer_Init(m_renderer);


	m_gameRunning = true;
	m_drawingBall = false;
	m_ballDrawnSuccessfully = false;

	m_drawingWall = false;
	m_wallDrawnSuccessfully = false;

	m_drawBallCheckBox = false;
	m_drawWallCheckBox = false;

	m_assetManager->Load(m_renderer, "Ball", "res/gfx/Ball.png");

	m_entities.push_back(std::make_unique<Ball>(*m_assetManager, Vector(480.f, 270.f), 100.f * 0.01f, 15.f));
//	m_entities.push_back(std::make_unique<Ball>(*m_assetManager, Vector(470.f, 100.f), 200.f * 0.01f));
}

Game::~Game()
{
	ImGui_ImplSDLRenderer_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(m_renderer);
	SDL_DestroyWindow(m_window);

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
				ImGui_ImplSDL2_ProcessEvent(&m_event);
				switch (m_event.type)
				{
					case SDL_QUIT:
						m_gameRunning = false;
						break;
					case SDL_MOUSEBUTTONDOWN:
						if (m_drawBallCheckBox)
						{
							if (!m_drawingBall && m_event.button.button == SDL_BUTTON_LEFT)
							{
								m_drawingBall = true;
								SDL_GetMouseState(&m_previousMouseX, &m_previousMouseY);
							}
							else if (m_drawingBall && m_event.button.button == SDL_BUTTON_LEFT)
							{
								m_ballDrawnSuccessfully = true;
							}
							if (m_drawingBall && m_event.button.button == SDL_BUTTON_RIGHT)
								m_drawingBall = false;
						}

						if (m_drawWallCheckBox)
						{
							if (!m_drawingWall && m_event.button.button == SDL_BUTTON_LEFT)
							{
								m_drawingWall = true;
								SDL_GetMouseState(&m_previousMouseX, &m_previousMouseY);
							}
							else if (m_drawingWall && m_event.button.button == SDL_BUTTON_LEFT)
								m_wallDrawnSuccessfully = true;
						}
						
						
						break;
					default:
						break;
				}

				for (const auto& entity : m_entities)
					entity->HandleEvents(m_event);

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

	for (const auto& entity : m_entities)
		entity->Update();

	CheckBallToBallCollision();
	CheckBallToWallCollision();

	if (m_drawingBall)
		DrawBallWithMouse();
	if (!m_drawBallCheckBox)
		m_drawingBall = false;

	//ImGui
	ImGui_ImplSDLRenderer_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Test");

	bool temp = false;
	temp = ImGui::Checkbox("Draw Ball", &m_drawBallCheckBox);
	if (temp)
		m_drawWallCheckBox = false;

	if (m_drawBallCheckBox)
		ImGui::Text("Left Click to draw ball");

	temp = ImGui::Checkbox("Draw Wall", &m_drawWallCheckBox);
	if (temp)
		m_drawBallCheckBox = false;

	if (m_drawWallCheckBox)
		ImGui::Text("Left Click to draw wall");

	ImGui::End();

	

}

void Game::Render()
{
	SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
	ImGui::Render();
	SDL_RenderClear(m_renderer);

	for (const auto& entity : m_entities)
	{
		const auto& render = dynamic_cast<const IRenderer *>(entity.get());

		if (render)
			render->Render(m_renderer);
	}

	SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);

	for (const auto& wall : m_walls)
	{
		const auto& render = dynamic_cast<const IRenderer*>(wall.get());

		if (render)
			render->Render(m_renderer);
	}

	if (m_drawingWall)
		DrawWall();

	if (!m_drawWallCheckBox)
		m_drawingWall = false;

	if (m_drawingBall)
	{
		const auto& r = dynamic_cast<const IRenderer*>(m_entity.get());
		if (r)
			r->Render(m_renderer);
	}

	ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());

	SDL_RenderPresent(m_renderer);
}

void Game::CheckBallToBallCollision()
{
	for (const auto& entity1 : m_entities)
	{
		for (const auto& entity2 : m_entities)
		{
			Ball* b1 = const_cast<Ball*>(dynamic_cast<const Ball*>(entity1.get()));
			Ball* b2 = const_cast<Ball*>(dynamic_cast<const Ball*>(entity2.get()));

			if (entity1 != entity2 && IsCollisionBetweenBalls(b1, b2))
			{
				BallCollision(b1, b2);
				CheckIfBallCollidesFurther(b1);
				CheckIfBallCollidesFurther(b2);
			}
		}
	}
}

bool Game::IsCollisionBetweenBalls(Ball* b1, Ball* b2)
{
	double dx = b1->GetPosition().GetX() - b2->GetPosition().GetX();
	double dy = b1->GetPosition().GetY() - b2->GetPosition().GetY();

	double dst = std::sqrt(dx * dx + dy * dy);
	double sumRadi = b1->GetRadius() + b2->GetRadius();

	if (sumRadi < dst)
		return false;

	return true;
}

void Game::BallCollision(Ball* b1, Ball* b2)
{
	Vector normalVector = b1->GetPosition() - b2->GetPosition();

	float dst = std::sqrt(normalVector.GetX() * normalVector.GetX() + normalVector.GetY() * normalVector.GetY());
	if (dst < 0.1f)
		dst = 0.1f;

	Vector normalVectorNormalized = Vector(normalVector.GetX() / dst, normalVector.GetY() / dst);

	Vector deltaVelocityVector = b1->m_velocity - b2->m_velocity;
	b1->m_velocity.SetX(b1->m_bounce * (b1->m_velocity.GetX() * (b1->m_mass - b2->m_mass) + (2 * b2->m_mass * b2->m_velocity.GetX())) / (b1->m_mass + b2->m_mass));
	b1->m_velocity.SetY(b1->m_bounce * (b1->m_velocity.GetY() * (b1->m_mass - b2->m_mass) + (2 * b2->m_mass * b2->m_velocity.GetY())) / (b1->m_mass + b2->m_mass));
	b2->m_velocity.SetX(-b2->m_bounce * (b2->m_velocity.GetX() * (b2->m_mass - b1->m_mass) + (2 * b1->m_mass * b1->m_velocity.GetX())) / (b1->m_mass + b2->m_mass));
	b2->m_velocity.SetY(-b2->m_bounce * (b2->m_velocity.GetY() * (b2->m_mass - b1->m_mass) + (2 * b1->m_mass * b1->m_velocity.GetY())) / (b1->m_mass + b2->m_mass));
	
	float travelDst = (0.08f + (b1->GetRadius() + b2->GetRadius())) - dst;

	b1->SetPosition(b1->GetPosition() + Vector(0.5f * travelDst * normalVectorNormalized.GetX(), 0.5f * travelDst * normalVectorNormalized.GetY()));
	b2->SetPosition(b2->GetPosition() + Vector(-0.5f * travelDst * normalVectorNormalized.GetX(), -0.5f * travelDst * normalVectorNormalized.GetY()));
}

void Game::CheckIfBallCollidesFurther(Ball* b)
{
	for (const auto& entity : m_entities)
	{
		Ball* b1 = const_cast<Ball*>(dynamic_cast<const Ball*>(entity.get()));

		if (b != b1 && IsCollisionBetweenBalls(b, b1))
		{
			BallCollision(b, b1);
			CheckIfBallCollidesFurther(b);
			CheckIfBallCollidesFurther(b1);
		}
	}
}

void Game::DrawBallWithMouse()
{
	int currentMouseX, currentMouseY;
	SDL_GetMouseState(&currentMouseX, &currentMouseY);

	int dx = currentMouseX - m_previousMouseX;
	int dy = currentMouseY - m_previousMouseY;

	float dst = (float)std::sqrt(dx * dx + dy * dy);

	m_entity = std::make_unique<Ball>(*m_assetManager, Vector((float)m_previousMouseX, (float)m_previousMouseY));

	Ball* b = const_cast<Ball*>(dynamic_cast<const Ball*>(m_entity.get()));
		

	//float tempScale;
	//if (dst <= 200.f)
		//tempScale = dst * 0.01f;
	//else
		//tempScale = 200 * 0.01f;

	if (dst >= 30)
		dst = 30;


	b->SetScale(dst / 30.f); // 30(ball png width height) , setting scale might break things
	b->m_radius = dst;


	if (m_ballDrawnSuccessfully)
	{
		m_entities.push_back(std::make_unique<Ball>(*m_assetManager, Vector((float)m_previousMouseX, (float)m_previousMouseY), dst / 30.f, dst));
		m_drawingBall = false;
		m_ballDrawnSuccessfully = false;
	}
}

void Game::DrawWall()
{
	int currentMouseX = 0, currentMouseY = 0;
	SDL_GetMouseState(&currentMouseX, &currentMouseY);

	SDL_RenderDrawLine(m_renderer, m_previousMouseX, m_previousMouseY, currentMouseX, currentMouseY);
	if (m_wallDrawnSuccessfully)
	{	
		int currentMouseX = 0, currentMouseY = 0;
		SDL_GetMouseState(&currentMouseX, &currentMouseY);

		//length of line
		float len = utils::Distance(Vector((float)m_previousMouseX, (float)m_previousMouseY), Vector((float)currentMouseX, (float)currentMouseY));

		//angle between x axis and line
		float angle = (float)std::atan2((currentMouseY - m_previousMouseY), currentMouseX - m_previousMouseX) * 180 / 3.14f;

		//center of line
		Vector m = Vector(((float)m_previousMouseX + (float)currentMouseX) / 2.f, ((float)m_previousMouseY + (float)currentMouseY) / 2.f);

		m_walls.push_back(std::make_unique<Wall>(m, Vector((float)m_previousMouseX, (float)m_previousMouseY), Vector((float)currentMouseX, (float)currentMouseY), angle, len));
		m_drawingWall = false;
		m_wallDrawnSuccessfully = false;
	}
}

void Game::CheckBallToWallCollision()
{
	for (const auto& entity : m_entities)
	{
		for (auto& wall : m_walls)
		{

			Ball* b = const_cast<Ball*>(dynamic_cast<const Ball*>(entity.get()));
			Wall* w = wall.get();

			if (IsCollisionBetweenBallAndWall(b, w))
			{
				CollideBallWithWall(b, w);
			}
		}
	}
}

bool Game::IsCollisionBetweenBallAndWall(Ball* b, Wall* w)
{
	Vector y = b->GetPosition() - w->m_position;
	Vector wallNorm = Vector(std::cos((w->m_angle * 3.14 / 180) + 3.14 / 2), std::sin((w->m_angle * 3.14 / 180) + 3.14 / 2));

	Vector u = Vector(std::cos(w->m_angle * 3.14 / 180), std::sin(w->m_angle * 3.14 / 180));

	double u_dot_y = y.GetX() * u.GetX() + y.GetY() * u.GetY();
	Vector y_proj_u = Vector(u_dot_y * u.GetX(), u_dot_y * u.GetY());

	if (std::sqrt(y_proj_u.GetX() * y_proj_u.GetX() + y_proj_u.GetY() * y_proj_u.GetY()) > w->m_length / 2 + b->GetRadius())
		return false;

	Vector y_hat = y - y_proj_u;

	if (std::sqrt(y_hat.GetX() * y_hat.GetX() + y_hat.GetY() * y_hat.GetY()) > b->GetRadius())
		return false;

	if (wallNorm.GetX() * y.GetX() + wallNorm.GetY() * y.GetY() <= 0)
		b->SetPosition(w->m_position + y_proj_u - Vector(std::cos((w->m_angle * 3.14 / 180) + 3.14 * 0.5) * b->GetRadius(), std::sin((w->m_angle * 3.14 / 180) + 3.14 * 0.5) * b->GetRadius()));
	else
		b->SetPosition(w->m_position + y_proj_u + Vector(std::cos((w->m_angle * 3.14 / 180) + 3.14 * 0.5) * b->GetRadius(), std::sin((w->m_angle * 3.14 / 180) + 3.14 * 0.5) * b->GetRadius()));

	return true;
}

void Game::CollideBallWithWall(Ball* b, Wall* w)
{
	Vector ballVel = b->m_velocity;
	Vector bVel_proj_wall_norm;
	Vector bVel_proj_wall_tang;

	Vector u = Vector(std::cos((w->m_angle * 3.14 / 180) + 3.14 / 2), std::sin((w->m_angle * 3.14 / 180) + 3.14 / 2));
	double u_dot_y = ballVel.GetX() * u.GetX() + ballVel.GetY() * u.GetY();
	bVel_proj_wall_norm = Vector(b->m_bounce * u_dot_y * u.GetX(), b->m_bounce * u_dot_y * u.GetY());

	u = Vector(std::cos(w->m_angle * 3.14 / 180), std::sin(w->m_angle * 3.14 / 180));
	u_dot_y = ballVel.GetX() * u.GetX() + ballVel.GetY() * u.GetY();
	bVel_proj_wall_tang = Vector(u_dot_y * u.GetX(), u_dot_y * u.GetY());

	b->m_velocity = bVel_proj_wall_tang - bVel_proj_wall_norm;
}