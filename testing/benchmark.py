from locust import HttpUser, task

class UserBehavior(HttpUser):

    @task
    def my_task(self):
        self.client.get("/")